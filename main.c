#include "dll.h"


int main(int argc, char *argv[]){
    printf("Test\n\n");

    const char *serialPortName = argv[1];
    const bool isTransmitter = argv[2][0] == '1'? 1 : 0;

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
        unsigned char data [] = "Hello";
        int error = llwrite(fd, data, sizeof(data));
        printf("%d\n", error);
        llclose(fd, isTransmitter);
    }else{
        int fd = llopen(serialPortName, isTransmitter);
        char data[MAX_SIZE] = {0};
        int bytes_read = llread(fd, data, MAX_SIZE);
        if(bytes_read < 0){
            printf("\tError llread\n");
            return -1;
        }
        printf("bytes_read: %d\n", bytes_read);
        
        printf("Data received: %s\n", data);
        llclose(fd, isTransmitter);
        
    }

    return 0;
}
