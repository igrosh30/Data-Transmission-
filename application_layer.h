#ifndef APPLICATION_LAYER_H
#define APPLICATION_LAYER_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

//include da DLLs
int sendFile();

int sendChunk(char chunk[]);

#endif