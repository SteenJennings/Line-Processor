// Name: Steinar Jennings
// Class: CS344
// Assignment: line_processor

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>


// char stringtest[10];
// char stringToBeCopied[] = "0123456789\n";
// printf("%d\n", sizeof(stringToBeCopied));
// strncpy(stringToBeCopied, stringtest, sizeof(stringToBeCopied));
// buffer1 variables
int inputLinesToProcess = 0;
char buffer1[49][1001]; // 49 lines of no more than 1000 characters
pthread_mutex_t mutex_pipe1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_input_full = PTHREAD_COND_INITIALIZER;
// buffer2 variables
int separatorLinesToProcess = 0;
char buffer2[49][1001]; // 49 lines of no more than 1000 characters
pthread_mutex_t mutex_pipe2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_separator_full = PTHREAD_COND_INITIALIZER;
int i = 0;
// buffer3 variables
int signCharsToProcess = 0;
char buffer3[49][1001]; // 49 lines of no more than 1000 characters
pthread_mutex_t mutex_pipe3 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_sign_full = PTHREAD_COND_INITIALIZER;



// this pipeline does NOT need to have a wait condition, because it cannot "overfill"
void * input_thread(void *args)
{
    int exitFlag = 0;                   // used to track if we are exiting this thread (after passing the STOP to the next thread)
    for(int i=0; i<49; i++)
    {
        fgets(buffer1[i], 1001, stdin); // gets the user input and stores it at the "i"th index of our buffer

        if (strcmp("STOP\n",buffer1[i])==0) // if we recieve the stop message, set our exit flag to be 1
        {
            exitFlag = 1;
        }
        pthread_mutex_lock(&mutex_pipe1);      // lock the mutex to increment our shared resource inputLinesToProcess
        inputLinesToProcess++;                 // incrementing because there is a new line to process
        pthread_mutex_unlock(&mutex_pipe1);    // unlock the mutex now that we are no longer using the shared resource
        pthread_cond_signal(&cond_input_full); // signal that we have added something to the shared resource, even if it's the STOP signal
        if(exitFlag == 1)               
        {  
            break;                             // if the STOP has been passed and the other threads have been updated the thread concludes
        }
    }
    return NULL;
}

// this pipeline with wait for its producer (inputThread) to create something to consume
void * separator_thread(void *args)
{
    int exitFlag = 0;
    char* line;                                     // this is declared out here becuase it is used at multiple levels below.

    for(int i=0; i<49; i++)
    {
        pthread_mutex_lock(&mutex_pipe1);           // locking the threads to evaluate IF there is input to process.
        if (inputLinesToProcess == 0)
        {
            pthread_cond_wait(&cond_input_full, &mutex_pipe1);  // if there is no input, wait until the condition is signaled
        }
        pthread_mutex_unlock(&mutex_pipe1);         // we unlock the mutex for pipe1 after we have checked for the input lines to process > 0, this will already be unlocked if we hit the cond 
        if (strcmp("STOP\n",buffer1[i])==0) // same exit logic as above, we flag it but still pass the STOP along
        {
            exitFlag = 1;
        }
        line = buffer1[i];
        line[strcspn(line, "\n")] = ' ';     // using strcspn will grab the first index of a '\n' and we then replace that index with a ' '
        strcpy(buffer2[i], line);           // we create a copy of the line variable and place it in the "i"th index of the new buffer
        pthread_mutex_lock(&mutex_pipe1);   // locking the threads to access shared resource, inputLinesToProcess
        inputLinesToProcess--;              // decrementing it because we have CONSUMED a line
        pthread_mutex_unlock(&mutex_pipe1);   
        pthread_mutex_lock(&mutex_pipe2);   // locking the second mutex to update a shared resource, separatorLinesToProcess
        separatorLinesToProcess++;          // incrementing because we have PRODUCED a new line, without an endline to the second buffer
        pthread_mutex_unlock(&mutex_pipe2);
        pthread_cond_signal(&cond_separator_full);  // signaling that there is a new line without and endline available for processing if the ^ processor is in wait
        if (exitFlag == 1)
        {
            break;  // exits the for loop and concludes the thread
        }
    }
    return NULL;
}

void * sign_thread(void *args)
{
    int exitFlag = 0;
    int k;                                              // this is declared here because it is used at multiple levels

    for(int i=0; i<49; i++)
    {
        pthread_mutex_lock(&mutex_pipe2);               // locking the threads to evaluate IF there is input to process
        if (separatorLinesToProcess == 0)
        {
            pthread_cond_wait(&cond_separator_full, &mutex_pipe2);  // if there is no input, wait until there is a separator line signaled
        }
        pthread_mutex_unlock(&mutex_pipe2);
        if (strcmp("STOP ",buffer2[i])==0)
        {
            exitFlag = 1;
        }
        k = 0;                      // initializing the index for the buffer3 copying, will vary from the other index when duplicate +'s are encountered
        for(int j=0; j< strlen(buffer2[i]); j++)
        {
            if(buffer2[i][j] == '+' && j < strlen(buffer2[i])-1 && (buffer2[i][j+1] == '+' ))
            {
                buffer3[i][k] = '^';        // we will change the index in buffer 3 at "k" because j will advance faster, in case of dups
                j++;                        // incrementing j twice during the loop because we are skipping over the next character if we found a dup
            }           
            else
            {
                buffer3[i][k] = buffer2[i][j];  // if there is not a dup, add the current character to the new buffer
            }
            k++;                                    // no matter what, k will only increment by 1 at a time
        }
        pthread_mutex_lock(&mutex_pipe3);           // accessing shared resource of the second buffer
        signCharsToProcess++;                       // incrementing since we have PRODUCED and new line
        pthread_cond_signal(&cond_sign_full);       // if something has been sent to the
        pthread_mutex_unlock(&mutex_pipe3);
        pthread_mutex_lock(&mutex_pipe2);           // accessing shared resource of the second buffer
        separatorLinesToProcess--;                  // decrementing it since we have CONSUMED a line
        pthread_mutex_unlock(&mutex_pipe2);
        if (exitFlag == 1)
        {
            break;
        }
    }
    return NULL;
}

// uses a local buffer to track when it has filled up 80 elements
void * output_thread(void *args)
{
    int exitFlag = 0;
    int n = 0;                                  // used to track our current pos in the local variable (so we dont need to dump mem)
    char local[81];                             // local buffer for counting 80 variables.
    for(int i=0; i<49; i++)
    {   
        pthread_mutex_lock(&mutex_pipe3);           // accessing shared resource of the second buffer
        if (signCharsToProcess == 0)
            pthread_cond_wait(&cond_sign_full, &mutex_pipe3);
        pthread_mutex_unlock(&mutex_pipe3);
        if (strcmp("STOP ",buffer3[i])==0)
        {   
            break;                                  // we can break early, because the stop does NOT need to be processed
        }         
        for(int j=0; j<strlen(buffer3[i]); j++)
        {
            local[n] = buffer3[i][j];
            n++;
            if(n == 80)                             // if our local index is pointed to the 81st character
            {
                local[n] = '\0';
                printf("%s\n",local);
                n = 0;                                                  
            }
        }
        pthread_mutex_lock(&mutex_pipe3);            // accessing shared resource of the second buffer
        signCharsToProcess--;                        // CONSUMES the line of input data, nothing is PRODUCED by this thread
        pthread_mutex_unlock(&mutex_pipe3);
    }
    return NULL;
}

// initialize our threads and join upon completion, in the proper order.
int main()
{
    pthread_t inputThread;
    pthread_t separatorThread;
    pthread_t signThread;
    pthread_t outputThread;
    pthread_create(&inputThread, NULL, input_thread, NULL);
    pthread_create(&separatorThread, NULL, separator_thread, NULL);
    pthread_create(&signThread, NULL, sign_thread, NULL);
    pthread_create(&outputThread, NULL, output_thread, NULL);
    pthread_join(inputThread, NULL);
    pthread_join(separatorThread, NULL);
    pthread_join(signThread, NULL);
    pthread_join(outputThread, NULL);
    return 0;
}
