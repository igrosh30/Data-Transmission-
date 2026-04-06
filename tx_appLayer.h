#ifndef TX_APPLAYER_H
#define TX_APPLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>   
#include "dll.h"

#define isTRANSMITER 1

void sendFileSerialLink(const char *serialPortName, const char *filename, 
                        int baudRate, int nTries, int timeout);


static int buildControlPacket(unsigned char controlByte, // Start: 2 End: 3 
                              unsigned char *packet, 
                              const char *filename, 
                              long fileSize);

static int buildDataPacket(unsigned char *packet, 
                           const unsigned char *data, 
                           int dataLen);


#endif