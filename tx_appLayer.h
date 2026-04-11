#include "rx_appLayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int parseControlPacket(const unsigned char *packet, int packetLen,
                              long *fileSize, char *filenameOut, int maxNameLen)
{
    if (packetLen < 1) return -1;
    unsigned char C = packet[0];
    if (C != 2 && C != 3) return -1;

    int idx = 1;
    if (idx + 2 >= packetLen || packet[idx] != 0) return -1;
    int L = packet[idx + 1];
    idx += 2;
    if (idx + L > packetLen) return -1;

    char sizeBuf[32] = {0};
    memcpy(sizeBuf, packet + idx, (L < 31 ? L : 31));
    *fileSize = atol(sizeBuf);
    idx += L;

    if (idx + 2 >= packetLen || packet[idx] != 1) return -1;
    L = packet[idx + 1];
    idx += 2;
    if (idx + L > packetLen) return -1;

    if (L >= maxNameLen) L = maxNameLen - 1;
    memcpy(filenameOut, packet + idx, L);
    filenameOut[L] = '\0';

    return C;
}

static int extractDataSegment(const unsigned char *packet, int packetLen,
                              unsigned char *outData, int maxData)
{
    if (packetLen < 4 || packet[0] != 1) return -1;
    uint16_t dataLen = ((uint16_t)packet[1] << 8) | packet[2];
    if (dataLen > packetLen - 3 || dataLen > maxData) return -1;
    memcpy(outData, packet + 3, dataLen);
    return dataLen;
}

void receiveFileSerialLink(const char *serialPortName, const char *filename,
                           int baudRate, int nTries, int timeout)
{
    FILE *file = NULL;
    int fd = -1;

    printf("=== Receiver Application Layer starting (baud = %d) ===\n", baudRate);

    DLLConfig config = { .baudrate = baudRate, .timeout = timeout, .numTries = nTries };

    fd = llopen(serialPortName, false, &config);
    if (fd < 0) {
        printf("ERROR: llopen failed (code %d)\n", fd);
        goto end;
    }
    printf("Connection established (fd = %d)\n", fd);

    unsigned char packet[512];
    bool transferStarted = false;

    while (1) {
        int bytes = llread(fd, (char*)packet, sizeof(packet));
        if (bytes < 0) {
            printf("ERROR in llread: %d\n", bytes);
            goto end;
        }
        if (bytes == 0) continue;

        unsigned char type = packet[0];

        if (type == 2) {
            long size = 0;
            char name[256] = {0};
            if (parseControlPacket(packet, bytes, &size, name, sizeof(name)) == 2) {
                file = fopen(filename, "wb");
                if (!file) { perror("fopen output"); goto end; }
                transferStarted = true;
                printf("START received → size %ld | name %s\n", size, name);
            }
        }
        else if (type == 1 && transferStarted) {
            unsigned char data[512];
            int len = extractDataSegment(packet, bytes, data, sizeof(data));
            if (len > 0) fwrite(data, 1, len, file);
        }
        else if (type == 3) {
            printf("END packet received → transfer complete\n");
            break;
        }
    }

end:
    if (file) fclose(file);
    if (fd >= 0) llclose(fd, false);
    printf("=== Receiver finished ===\n");
}