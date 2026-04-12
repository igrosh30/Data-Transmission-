#include "tx_appLayer.h"
#include <time.h>
#include <sys/time.h>

int retransmission_count = 0;

static int buildControlPacket(unsigned char controlByte, 
                              unsigned char *packet, 
                              const char *filename, 
                              long fileSize)
{
    int idx = 0;
    packet[idx++] = controlByte;

    char sizeBuf[32]; 
    int sizeLen = sprintf(sizeBuf, "%ld", fileSize);
    
    packet[idx++] = 0;
    packet[idx++] = (unsigned char)sizeLen;
    memcpy(packet + idx, sizeBuf, sizeLen);
    idx += sizeLen;

    int nameLen = strlen(filename);
    packet[idx++] = 1;
    packet[idx++] = (unsigned char)nameLen;
    memcpy(packet + idx, filename, nameLen);
    idx += nameLen;

    return idx;
}

static int buildDataPacket(unsigned char *packet, 
                           const unsigned char *data, 
                           int dataLen)
{
    int idx = 0;
    packet[idx++] = 1;

    uint16_t k = (uint16_t)dataLen;
    packet[idx++] = (k >> 8) & 0xFF;
    packet[idx++] = k & 0xFF;

    memcpy(packet + idx, data, dataLen);
    idx += dataLen;

    return idx;
}

void sendFileSerialLink(const char *serialPortName, const char *filename, 
                        int baudRate, int nTries, int timeout, 
                        int numRuns, double fer)
{
    FILE *csv = fopen("results.csv", "a");
    if (csv && ftell(csv) == 0) {
        fprintf(csv, "run,baudrate,fer,file_name,file_size_bytes,transfer_time_sec,throughput_bps,retransmissions\n");
    }

    for (int run = 1; run <= numRuns; run++) {
        printf("\n=== Run %d / %d | Baud: %d | FER: %.3f ===\n", run, numRuns, baudRate, fer);

        retransmission_count = 0;

        struct timeval startTime, endTime;
        gettimeofday(&startTime, NULL);

        FILE *file = fopen(filename, "rb");
        if (!file) {
            perror("fopen");
            continue;
        }
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        rewind(file);

       
        DLLConfig config = { .baudRate = baudRate, .timeout = timeout, .numTries = nTries };
        int fd = llopen(serialPortName, true, &config);

        if (fd < 0) {
            printf("ERROR: llopen failed\n");
            fclose(file);
            continue;
        }

        unsigned char packet[1024];
        int len = buildControlPacket(2, packet, filename, fileSize);
        llwrite(fd, packet, len);

        #define MAX_DATA_SIZE 900
        unsigned char dataBuf[MAX_DATA_SIZE];
        size_t bytesRead;
        while ((bytesRead = fread(dataBuf, 1, MAX_DATA_SIZE, file)) > 0) {
            len = buildDataPacket(packet, dataBuf, (int)bytesRead);
            llwrite(fd, packet, len);
        }

        len = buildControlPacket(3, packet, filename, fileSize);
        llwrite(fd, packet, len);

        llclose(fd, true);
        fclose(file);

        gettimeofday(&endTime, NULL);
        double time_sec = (endTime.tv_sec - startTime.tv_sec) + 
                          (endTime.tv_usec - startTime.tv_usec) / 1000000.0;

        double throughput = (time_sec > 0) ? (fileSize / time_sec) : 0.0;

        printf("Run %d finished → Time: %.3fs | Throughput: %.0f B/s | Retransmissions: %d\n",
               run, time_sec, throughput, retransmission_count);

        if (csv) {
            fprintf(csv, "%d,%d,%.3f,%s,%ld,%.3f,%.0f,%d\n",
                    run, baudRate, fer, filename, fileSize, time_sec, throughput, retransmission_count);
        }
    }

    if (csv) fclose(csv);
    printf("\n=== All %d runs finished. Results saved in results.csv ===\n", numRuns);
}
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