//#include <stdio.h>

#include "stateMachine.h"

uint8_t received_control_byte = 0;

void init()
{
    //maybe update or set the state 
}

STATE updateSupervisionFrame(uint8_t byte, STATE st, bool isTx)
{
    //VER expectedAdress!
    uint8_t expectedAddress = isTx ? TRANSMITER: RECEIVER;       
    
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
            else if(byte == expectedAddress)
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
            else if(isValidControlByte(byte))
            {
                received_control_byte = byte;
    
                st = C_RCV;
            }
            else st = STATE_START;
            break;

        case C_RCV:
            if(byte == FLAG)
            {
                st = FLAG_RCV;
                received_control_byte = 0;
            }
            else if(byte == (expectedAddress ^ received_control_byte))
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

// Helper function to check if a byte is a valid control field
bool isValidControlByte(uint8_t byte) {
    switch(byte) {
        case SET:
        case UA:
        case RR0:
        case RR1:
        case REJ0:
        case REJ1:
        case DISC:
        case IF0:
        case IF1:
            return true;
        default:
            return false;
    }
}


STATE updateIFrame(uint8_t byte, STATE st)
{
    //VER expectedAdress!
    uint8_t expectedAddress = TRANSMITER;       
    
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
            else if(byte == expectedAddress)
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
            else if(isValidControlByte(byte))
            {
                received_control_byte = byte;
    
                st = C_RCV;
            }
            else st = STATE_START;
            break;

        case C_RCV:
            if(byte == FLAG)
            {
                st = FLAG_RCV;
                received_control_byte = 0;
            }
            else if(byte == (expectedAddress ^ received_control_byte))
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
            break;
    }   
    return st;  
}