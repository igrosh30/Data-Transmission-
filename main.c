#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tx_appLayer.h"
#include "rx_appLayer.h"

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("Usage:\n");
        printf("  Transmitter: %s <port> 1 <filename> <baudRate> <numRuns> <fer>\n", argv[0]);
        printf("  Receiver:    %s <port> 0 <outputFilename>\n", argv[0]);
        printf("Example: %s /dev/ttyUSB0 1 penguin.gif 115200 8 0.0\n", argv[0]);
        return 1;
    }

    const char *serialPort = argv[1];
    int isTransmitter = atoi(argv[2]);
    const char *filename = argv[3];

    if (isTransmitter) {
        int baudRate = (argc >= 5) ? atoi(argv[4]) : 38400;
        int numRuns  = (argc >= 6) ? atoi(argv[5]) : 1;
        double fer   = (argc >= 7) ? atof(argv[6]) : 0.0;

        printf("=== TRANSMITTER - baud %d | %d runs | FER %.3f ===\n\n", baudRate, numRuns, fer);
        sendFileSerialLink(serialPort, filename, baudRate, 3, 3, numRuns, fer);
    } 
    else {
        printf("=== RECEIVER MODE ===\n");
        receiveFileSerialLink(serialPort, filename, 38400, 3, 3);
    }

    return 0;
}