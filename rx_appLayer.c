/*
1- we have a hard Coded BaudRate! 
2- we are passing the role- this means ti should he same file for both! 

*/
#include "rx_appLayer.h"


void receiveFileSerialLink(const char *serialPortName, const char *filename, 
                        int baudRate, int nTries, int timeout)
{ 
    FILE *file = NULL;
    file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        return;                     
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);
    if(fileSize < 0) 
    {
        perror("ftell");
        fclose(file);
        return;
    }
    printf("File opened successfully - size = %ld bytes\n", fileSize);

    DLLConfig config;
    config.baudrate = baudRate;
    config.timeout = timeout;
    config.numTries = nTries;

    int fd = llopen(serialPortName, TRANSMITER, &config);  

    if (fd < 0) {
        printf("ERROR: %d\n",fd);
        goto end;
    }
    printf("Connection established (fd = %d)\n", fd);

    //ALL ok, let's build start packet! 
    unsigned char packet[512];
    int len = buildControlPacket(2,packet, filename, fileSize);
    int res_start = llwrite(fd, packet, len);
    if ( res_start < 0) {
        printf("ERROR sending START packet: %d\n",res_start);
        goto end;
    }

    //all ok, let's send the data! 
    #define MAX_DATA_SIZE 200
    unsigned char dataBuf[MAX_DATA_SIZE];
    size_t bytesRead;

    while ((bytesRead = fread(dataBuf, 1, MAX_DATA_SIZE, file)) > 0) {
        len = buildDataPacket(packet, dataBuf, (int)bytesRead);
        int res_data = llwrite(fd, packet, len);
        if ( res_data < 0) {
            printf("ERROR sending DATA packet - aborting: %d\n", res_data);
            goto end;
        }
    }

    len = buildControlPacket(3, packet,filename, fileSize);
    if (llwrite(fd, packet, len) < 0) printf("ERROR sending END packet\n");
    else printf("END packet sent successfully\n");

end:
    if (fd >= 0)      llclose(fd, 0);
    if (file != NULL) fclose(file);
    printf("=== File transfer finished ===\n");
    
}


static int buildControlPacket(unsigned char controlByte, 
                              unsigned char *packet, 
                              const char *filename, 
                              long fileSize)
{
    int idx = 0;
    packet[idx++] = controlByte;          // C = 2 (START) or C = 3 (END)
    
    // TLV: File size (T=0, V = ASCII decimal string)
    char sizeBuf[32]; 
    int sizeLen = sprintf(sizeBuf, "%ld", fileSize);
    
    packet[idx++] = 0;                    // T = 0 (size)
    packet[idx++] = (unsigned char)sizeLen;
    memcpy(packet + idx, sizeBuf, sizeLen);
    idx += sizeLen;

    // TLV: File name (T=1)
    int nameLen = strlen(filename);
    packet[idx++] = 1;                    // T = 1 (name)
    packet[idx++] = (unsigned char)nameLen;
    memcpy(packet + idx, filename, nameLen);
    idx += nameLen;

    return idx;   // total length of the packet
}


static int buildDataPacket(unsigned char *packet, 
                           const unsigned char *data, 
                           int dataLen)
{
    //TENHO DE VER AINDA MELHOR!
    int idx = 0;

    packet[idx++] = 1;                    // C = 1 → DATA

    uint16_t k = (uint16_t)dataLen;
    packet[idx++] = (k >> 8) & 0xFF;      // L2 (high byte)
    packet[idx++] = k & 0xFF;             // L1 (low byte)

    memcpy(packet + idx, data, dataLen);
    idx += dataLen;

    return idx;
}
