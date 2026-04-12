#include "rx_appLayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef APP_LAYER_H
#define APP_LAYER_H

#include <stdbool.h>

// Transmitter
void sendFileSerialLink(const char *serialPortName, const char *filename, 
                        int baudRate, int nTries, int timeout, 
                        int numRuns, double fer);


#endif