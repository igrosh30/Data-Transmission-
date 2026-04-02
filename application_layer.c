#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define CHUNK_SIZE 1500 //bytes
//include da DLL
int sendFile(){
    FILE* fptr;

    // Opening the file in read mode
    fptr = fopen("filename.txt", "r");

    // checking if the file is 
    // opened successfully
    if (fptr == NULL) {
        printf("The file is not opened.");
    }

    //fd = llopen()

    char data[CHUNK_SIZE];

    int error;
    while (fgets(data, CHUNK_SIZE, fptr)!= NULL && error == 0) {
        error = sendChunk(data);
    }

    // Closing the file using fclose()
    fclose(fptr);
}

int sendChunk(char chunk[]){
   //llwrite()
}

