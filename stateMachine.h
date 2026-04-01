#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include "Config.h"

uint8_t frameReady;

typedef enum{
    STATE_START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP
}STATE;

void init();
STATE updateSupervisionFrame(uint8_t byte, STATE st,bool isTx);

#endif