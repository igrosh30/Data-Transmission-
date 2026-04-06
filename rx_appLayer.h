#ifndef RX_APPLAYER_H
#define RX_APPLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "dll.h"

void receiveFileSerialLink(const char *serialPortName, const char *filename,
                           int baudRate, int nTries, int timeout);

#endif