#include "dll.h"


int main(int argc, char *argv[]){
    printf("Test\n");

    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
        return 1;
    }

    llopen(serialPortName, true);

    return 0;
}
