#include "rx_appLayer.h"
#include "tx_appLayer.h"


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
    printf("Transmitter?: %d\n", isTransmitter);
    if(isTransmitter){
        sendFileSerialLink(serialPortName, "penguin.gif", BAUDRATE, 3, 3);
    }else{
        receiveFileSerialLink(serialPortName, "file_to_receive.gif", BAUDRATE, 3, 3);
    }


    return 0;
}
