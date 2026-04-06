/*
1- we have a hard Coded BaudRate! 
2- we are passing the role- this means ti should he same file for both! 

*/
#include "rx_appLayer.h"

void receiveFileSerialLink(const char *serialPortName, char *filename, 
                        int baudRate, int nTries, int timeout) {

    DLLConfig config;
    config.baudrate = baudRate;
    config.timeout = timeout;
    config.numTries = nTries;
    
    int fd = llopen(serialPortName, 0, &config);
    if (fd < 0) {
        perror("llopen");
        return;
    }
    printf("Connection established (fd = %d). Waiting for START packet...\n", fd);

    unsigned char packet[1024];
    long fileSize = 0;
    FILE *file = NULL;
    int transferFinished = 0;

    while (!transferFinished) {
        int bytesRead = llread(fd, packet, 1024);
        if (bytesRead < 0) {
            printf("Error reading from link layer\n");
            break;
        }
        if (bytesRead == 0) continue;

        unsigned char controlByte = packet[0];

        if (controlByte == 2 || controlByte == 3) { // START (2) or END (3) Control Packet
            int idx = 1;
            while (idx < bytesRead) {
                unsigned char type = packet[idx++];
                unsigned char length = packet[idx++];
                
                if (type == 0) { // T=0: File Size
                    char sizeBuf[32];
                    memcpy(sizeBuf, packet + idx, length);
                    sizeBuf[length] = '\0';
                    fileSize = atol(sizeBuf);
                    printf("Control Packet: File Size = %ld bytes\n", fileSize);
                } 
                else if (type == 1) { // T=1: File Name
                    memcpy(filename, packet + idx, length);
                    filename[length] = '\0';
                    printf("Control Packet: File Name = %s\n", filename);
                }
                idx += length;
            }

            if (controlByte == 2) { // START
                file = fopen(filename, "wb");
                if (!file) {
                    perror("fopen");
                    llclose(fd, 0);
                    return;
                }
                printf("File created. Starting data reception...\n");
            } else { // END
                printf("END packet received.\n");
                transferFinished = 1;
            }
        } 
        else if (controlByte == 1) { // DATA Packet
            if (file == NULL) {
                printf("Error: Received DATA packet before START packet.\n");
                continue;
            }
            // Parse L2 L1 (k = 256 * L2 + L1)
            uint16_t k = (packet[1] << 8) | packet[2];
            
            // Write payload data to file [cite: 381]
            size_t written = fwrite(packet + 3, 1, k, file);
            if (written != k) {
                printf("Error writing data to file.\n");
            }
        }
    }

    if (file) fclose(file);
    llclose(fd, 0);
    printf("=== File transfer finished ===\n");
}