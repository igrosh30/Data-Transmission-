#ifndef DLL_H
#define DLL_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "alarm_sigaction.h" 
#include "stateMachine.h"

#define MAX_SIZE 256
#define BAUDRATE B38400

#define TIMEOUT_RECEIVER 3 //seconds

#define MAX_ALARM_COUNT_RX 4 //

//static int serial_fd = -1;

struct linkLayer {
    char port[20]; /*Device /dev/ttySx, x = 0, 1*/
    int baudRate; /*Speed of the transmission*/
    unsigned int sequenceNumber; /*Frame sequence number: 0, 1*/
    unsigned int timeout; /*Timer value: 1 s*/
    unsigned int numTransmissions; /*Number of retries in case of failure*/
    char frame[MAX_SIZE]; /*Frame*/
};

int setup_termios(int fd);

int llclose(int fd, bool isTransmitter);

int llwrite(int fd, const unsigned char *buf, int bufSize);

int llopen(const char serialPortName[], bool isTransmitter);

int llread(int fd, char* buf, uint16_t size_buf);

int wait_SET(int fd);

int send_set_N_wait_UA(int fd);
int send_disc_N_wait_DISC(int fd);
int wait_DISC(int fd);

int send_UA(int fd);

int send_DISC(int fd);

int wait_UA(int fd);

#endif