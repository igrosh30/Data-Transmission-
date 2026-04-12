#include <stdio.h>
#include <stdlib.h>

#include "tx_appLayer.h"
#include "rx_appLayer.h"

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("Usage:\n");
        printf("  Transmitter sweep: %s <port> 1 <filename> <minBaud> <maxBaud> <step> <runsPerBaud> <fer>\n", argv[0]);
        printf("  Receiver:          %s <port> 0 <outputFilename> [baudRate]\n", argv[0]);
        printf("Example: %s /dev/ttyS10 1 penguin.gif 38400 115200 19200 5 0.05\n", argv[0]);
        return 1;
    }

    const char *serialPort = argv[1];
    int isTransmitter = atoi(argv[2]);
    const char *filename = argv[3];

    if (isTransmitter) {
        int minBaud     = (argc >= 5) ? atoi(argv[4]) : 38400;
        int maxBaud     = (argc >= 6) ? atoi(argv[5]) : 38400;
        int step        = (argc >= 7) ? atoi(argv[6]) : 19200;
        int runsPerBaud = (argc >= 8) ? atoi(argv[7]) : 1;
        double fer      = (argc >= 9) ? atof(argv[8]) : 0.0;

        if (minBaud > maxBaud) { int t = minBaud; minBaud = maxBaud; maxBaud = t; }
        if (step <= 0) step = 19200;
        if (runsPerBaud < 1) runsPerBaud = 1;
        if (fer < 0.0) fer = 0.0;
        if (fer > 1.0) fer = 1.0;

        printf("=== TRANSMITTER SWEEP MODE ===\n");
        printf("Baud range: %d → %d (step %d) | Runs per baud: %d | FER: %.3f\n\n",
               minBaud, maxBaud, step, runsPerBaud, fer);

        for (int baud = minBaud; baud <= maxBaud; baud += step) {
            sendFileSerialLink(serialPort, filename, baud, 3, 3, runsPerBaud, fer);
        }
    }
    else {
        int baudRate = (argc >= 5) ? atoi(argv[4]) : 38400;
        printf("=== RECEIVER MODE (baud = %d) ===\n", baudRate);
        receiveFileSerialLink(serialPort, filename, baudRate, 3, 3);
    }

    return 0;
}