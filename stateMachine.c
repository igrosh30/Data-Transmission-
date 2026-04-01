//#include <stdio.h>

#include "stateMachine.h"


void init()
{
    //maybe update or set the state 
}

STATE updateSupervisionFrame(uint8_t byte, STATE st, bool isTx)
{
    uint8_t expected_C   = isTx ? 0x07 : 0x03;          
    uint8_t expected_BCC = TRANSMITER ^ expected_C; 
    switch (st){

        case STATE_START:
            if(byte == FLAG){
                st= FLAG_RCV;
            }
            break;

        case FLAG_RCV:
            if(byte== FLAG)
            {
                st= FLAG_RCV;
            } 
            else if(byte == TRANSMITER)
            {
                st = A_RCV;
            } 
            else{
                st = STATE_START;
            }
            break;
        case A_RCV:

            if(byte == FLAG)
            {
                st = FLAG_RCV;
            }
            else if(byte == expected_C)
            {
                st = C_RCV;
            }
            else st = STATE_START;
            break;

        case C_RCV:
            if(byte == FLAG)
            {
                st = FLAG_RCV;
            }
            else if(byte == expected_BCC)
            {
                st = BCC_OK;
            }
            else {
                st = STATE_START;
            }
            break;
        case BCC_OK:
            if(byte == FLAG){
                st = STOP;
            }
            else{
                st = STATE_START;
            }
            break;
    }   
    return st;  
}
