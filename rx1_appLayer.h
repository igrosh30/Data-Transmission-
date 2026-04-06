#ifndef RX_APPLAYER_H
#define RX_APPLAYER_H

void receiveFileSerialLink(const char *serialPortName, const char *filename,
                           int baudRate, int nTries, int timeout);

#endif