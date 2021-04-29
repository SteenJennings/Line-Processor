Enter the command gcc --std=gnu99 -pthread -o line_processor line_processor.c to compile the file.
Once the file is successfully compiled, it can be run with or without input. 
To run the program with input: ./line_processor < input1.txt (where input1.txt is your input file)
To run the program to an outfile: ./line_processor > output1.txt (where output1.txt is your output file)
The program will take up to 49 inputs 
The program will process the inputs and remove newlines and replace them with " "
The program will remove instances of "++" and change it to "^"
The program will output to STDOUT the input lines everytime 80 characters have been processed.
The program will exit after it receives the input line "STOP" OR after receiving 49 inputs and processing.