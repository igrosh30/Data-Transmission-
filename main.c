#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tx_appLayer.h"
#include "rx_appLayer.h"

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("Usage:\n");
        printf("  Transmitter: %s <port> 1 <filename> <baudRate> [numRuns] [fer] [dataSize]\n", argv[0]);
        printf("  Receiver:    %s <port> 0 <outputFilename> <baudRate>\n", argv[0]);
        printf("Example:\n");
        printf("  Transmitter: %s /dev/ttyUSB0 1 penguin.gif 115200 1 0.0 800\n", argv[0]);
        printf("  Receiver:    %s /dev/ttyUSB1 0 received_penguin.gif 115200\n", argv[0]);
        return 1;
    }

    const char *serialPort = argv[1];
    int isTransmitter = atoi(argv[2]);
    const char *filename = argv[3];

    if (isTransmitter) {
        int baudRate = (argc >= 5) ? atoi(argv[4]) : 38400;
        int numRuns  = (argc >= 6) ? atoi(argv[5]) : 1;
        double fer   = (argc >= 7) ? atof(argv[6]) : 0.0;
        int dataSize = (argc >= 8) ? atoi(argv[7]) : 900; // <--- Parse dataSize

        printf("=== TRANSMITTER - baud %d | %d runs | FER %.3f | dataSize %d ===\n\n", 
               baudRate, numRuns, fer, dataSize);

        // Pass dataSize at the end of your function call (using 3 for nTries and timeout)
        sendFileSerialLink(serialPort, filename, baudRate, 3, 3, numRuns, fer, dataSize);
    } 
    else {
        int baudRate = (argc >= 5) ? atoi(argv[4]) : 38400;
        printf("=== RECEIVER - baud %d ===\n\n", baudRate);
        // Note: Make sure receiveFileSerialLink matches its header!
        receiveFileSerialLink(serialPort, filename, baudRate, 3, 3);
    }

    return 0;
}