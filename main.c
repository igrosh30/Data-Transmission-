#include "dll.h"


int main(int argc, char *argv[]){
    printf("Test\n\n");

    const char *serialPortName = argv[1];
    const bool isTransmitter = argv[2];

    if (argc < 3)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1 1|0\n",
               argv[0],
               argv[0]);
        exit(1);
        return 1;
    }

    if(isTransmitter){
        int fd = llopen(serialPortName, isTransmitter);
        char data [] = "Hello";
        llwrite(fd, data, sizeof(data));
    }else{
        int fd = llopen(serialPortName, isTransmitter);
        char data[MAX_SIZE];
        llread(fd, data);
    }

    return 0;
}
