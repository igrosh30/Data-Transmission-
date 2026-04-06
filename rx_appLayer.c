/*
 * Receiver Application Layer
 * Follows Slide 22 exactly (General operation of the Rx application layer)
 */

#include "rx_appLayer.h"   // you will create this header (see below)


// ================================================
// Helper: parse START (C=2) or END (C=3) control packet
// Returns 2=START, 3=END, or -1 on error
// ================================================
static int parseControlPacket(const unsigned char *packet, int packetLen,
                              long *fileSize, char *filenameOut, int maxNameLen)
{
    if (packetLen < 1) return -1;

    unsigned char C = packet[0];
    if (C != 2 && C != 3) return -1;

    int idx = 1;

    // TLV: File size (T=0)
    if (idx + 2 >= packetLen || packet[idx] != 0) return -1;
    int L = packet[idx + 1];
    idx += 2;
    if (idx + L > packetLen) return -1;

    char sizeBuf[32] = {0};
    memcpy(sizeBuf, packet + idx, (L < 31 ? L : 31));
    *fileSize = atol(sizeBuf);
    idx += L;

    // TLV: File name (T=1)
    if (idx + 2 >= packetLen || packet[idx] != 1) return -1;
    L = packet[idx + 1];
    idx += 2;
    if (idx + L > packetLen) return -1;

    if (L >= maxNameLen) L = maxNameLen - 1;
    memcpy(filenameOut, packet + idx, L);
    filenameOut[L] = '\0';

    return C;   // 2 = START, 3 = END
}

// ================================================
// Helper: extract data segment from DATA packet (C=1)
// Returns number of data bytes or -1 on error
// ================================================
static int extractDataSegment(const unsigned char *packet, int packetLen,
                              unsigned char *outData, int maxData)
{
    if (packetLen < 4 || packet[0] != 1) return -1;

    uint16_t dataLen = ((uint16_t)packet[1] << 8) | packet[2];

    if (dataLen > packetLen - 3 || dataLen > maxData) return -1;

    memcpy(outData, packet + 3, dataLen);
    return dataLen;
}

// ================================================
// Main receiver function (exactly as in slide 22)
// ================================================
void receiveFileSerialLink(const char *serialPortName, const char *filename,
                           int baudRate, int nTries, int timeout)
{
    FILE *file = NULL;
    int fd = -1;

    
    printf("Port: %s | Output file: %s | baud=%d | tries=%d | timeout=%d\n",
           serialPortName, filename, baudRate, nTries, timeout);

    // 1. Prepare config (same as transmitter)
    DLLConfig config;
    config.baudrate = baudRate;
    config.timeout  = timeout;
    config.numTries = nTries;

    // 2. Invoke llopen (receiver = false)
    fd = llopen(serialPortName, false, &config);
    if (fd < 0) {
        printf("ERROR: llopen failed (code %d)\n", fd);
        goto end;
    }
    printf("Connection established (fd = %d)\n", fd);

    unsigned char packet[512];
    bool transferStarted = false;
    long expectedSize = 0;
    char receivedName[256] = {0};

    // 3. Main loop (slide 22)
    while (1) {
        // Invoke llread
        int bytes = llread(fd, (char*)packet, sizeof(packet));

        if (bytes < 0)
        {
            printf("ERROR in llread: %d\n", bytes);
            goto end;
        }
        if (bytes == 0) continue;

        unsigned char type = packet[0];

        if (type == 2) {                    // START packet
            long sizeFromPkt = 0;
            char nameFromPkt[256] = {0};

            if (parseControlPacket(packet, bytes, &sizeFromPkt, nameFromPkt, sizeof(nameFromPkt)) == 2)
            {
                expectedSize = sizeFromPkt;
                strcpy(receivedName, nameFromPkt);

                if (!transferStarted)
                {
                    file = fopen(filename, "wb");   // create/overwrite output file
                    if (!file) 
                    {
                        perror("fopen output file");
                        goto end;
                    }
                    transferStarted = true;
                    printf("START packet received → File size: %ld | Name: %s\n",
                           sizeFromPkt, nameFromPkt);
                }
            }
        }
        else if (type == 1) // DATA packet
        {               
            if (!transferStarted) 
            {
                printf("DATA received before START → ignoring\n");
                continue;
            }

            unsigned char dataBuf[512];
            int dataLen = extractDataSegment(packet, bytes, dataBuf, sizeof(dataBuf));

            if (dataLen > 0)
            {
                fwrite(dataBuf, 1, dataLen, file);
                printf("Received %d data bytes\n", dataLen);
            }
        }
        else if (type == 3)
        {               // END packet
            printf("END packet received\n");
            break;   // exit loop → go to cleanup
        }
        else printf("Unknown packet type 0x%02X → ignoring\n", type);
        
    }

end:
    // Single cleanup point (same style as your transmitter)
    if (file != NULL) {
        fclose(file);
        printf("Output file closed\n");
    }
    if (fd >= 0) {
        llclose(fd, false);   // false = receiver
    }
    printf("=== File transfer finished (receiver) ===\n");
}