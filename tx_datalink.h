#ifndef TX_DATALINK_H
#define TX_DATALINK_H

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

struct termios oldtio;
STATE current_state = STATE_START;

int setup_termios(int fd);
int llopen1(int argc, char *argv[]);
int llopen(int porta, int role);
int llwrite(int fd, const unsigned char *buf, int bufSize);
int llclose(int fd);

#endif