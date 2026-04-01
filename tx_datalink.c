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

/*
llopen
» opens serial port (invokes openSerialPort)
» sends SET frame
» reads one byte at a time to receive UA frame (see state machine in slide 40)
» returns success or failure*/
//NOTE: int argc char *argv -> this will be called passing in main! for the appLication layer! 
//FINAL FUNCION: int llopen(int porta, TRANSMITTER | RECEIVER)-TRANS/RECEIVER is a flag! 
int llopen(int argc, char *argv[])
{
    //openSerialPort:
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
        return 1;
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        return -1;
    }

    if (setup_termios(fd) == -1) return -1; 
    
    unsigned char setFrame[5] = {
        FLAG,
        TRANSMITER,
        SET,                   
        TRANSMITER ^ SET,
        FLAG
    };

    int bytes = write(fd, setFrame, 5);
    printf("%d bytes written\n", bytes);

    printf("Sent data... waiting\n");

    setup();//setup the alarm! 

    unsigned char bufR[MAX_SIZE] = {0};
    
    alarmCount = 0;
    alarmEnabled = FALSE;
    current_state = STATE_START;
    while (alarmCount < 4 && current_state != STOP)//read 5 bytes! 
    {
        if (alarmEnabled == FALSE) {
            alarm(3);
            alarmEnabled = TRUE;

            if (alarmCount > 0) { 
                write(fd, setFrame, 5);
                printf("Timeout → SET frame resent (retry %d/3)\n", alarmCount);
            }
        }

        uint8_t byte = 0;
        if(read(fd, &byte, 1) > 0)
        {
            current_state = updateSupervisionFrame(byte, current_state, true);
            if(current_state == STOP)
            {
                if(received_control_byte == UA)
                {
                    alarm(0);
                    return fd; 
                }
            } 
        } 
    }
    //failed 
    printf("Failed to receive UA after %d retries\n", alarmCount);
    close(fd);
    return -1;  
}


/*
 llwrite
» implements error detection (compute BCC over the data packet)
» parses data packet to implement byte stuffing (transparency)
» builds Information frame Ns (Ns=0 or 1)
» sends I frame - this goes defined in the control field! 
» reads one byte at a time to receive response
» if negative response (REJ) or no response, resends I frame up to a maximum number of times
(retransmission mechanism explained in slide 46)
» return success or failure
*/
int llwrite(int fd, const unsigned char *buf, int bufSize)
{
    
    if (fd < 0 || buf == NULL || bufSize <= 0) return -1;

    static int Ns = 0;
    uint8_t bcc2 = 0;
    
    for (int i = 0; i < bufSize; i++) bcc2 ^= buf[i];

    unsigned char stuffed[MAX_SIZE * 2];
    int stuffedLen = 0;

    for (int i = 0; i < bufSize; i++) {
        if (buf[i] == FLAG || buf[i] == 0x7D) {
            stuffed[stuffedLen++] = 0x7D;
            stuffed[stuffedLen++] = (buf[i] == FLAG) ? 0x5E : 0x5D;
        } else {
            stuffed[stuffedLen++] = buf[i];
        }
    }

    /*now I have the dataReady:
    I have the Ns ready now we need to send it! - where goes Ns?! ...
    • C (Control) to distinguish between the different types of supervision 
        frames and to carry sequence numbers N(s) in I frames
        • Control field - value 0x00 indicates that it is the Information frame number 0
        • Control field - value 0x40 indicates that it is the Information frame number 1
    */


    //BUILD THE I FRAME FIRST! 
    unsigned char I_frame[MAX_SIZE * 2 + 10];
    int frameLen = 0;

    uint8_t C    = (Ns == 0) ? 0x00 : 0x40;
    uint8_t BCC1 = TRANSMITER ^ C;

    I_frame[frameLen++] = FLAG;
    I_frame[frameLen++] = TRANSMITER;   
    I_frame[frameLen++] = C;            
    I_frame[frameLen++] = BCC1;         

    memcpy(&I_frame[frameLen], stuffed, stuffedLen); frameLen += stuffedLen;

    if (bcc2 == FLAG || bcc2 == 0x7D) {
        I_frame[frameLen++] = 0x7D;
        I_frame[frameLen++] = (bcc2 == FLAG) ? 0x5E : 0x5D;
    } else {
        I_frame[frameLen++] = bcc2;
    }
    I_frame[frameLen++] = FLAG;

    int bytes_write = write(fd,I_frame,frameLen);
    if (bytes_write != frameLen) return -1;

    alarmCount = 0;
    alarmEnabled = FALSE;
    current_state = STATE_START;
    while (alarmCount < 4 && current_state != STOP)
    {
        if (alarmEnabled == FALSE) {
            alarm(3);
            alarmEnabled = TRUE;

            if (alarmCount > 0) { 
                write(fd, I_frame, frameLen);
            }
        }

        //Do the reading:
        uint8_t byte = 0;
        
        if(read(fd, &byte, 1) > 0)
        {
            uint8_t expectedRR = (Ns == 0) ? RR1 : RR0;//receiver got message 0 - send ready for 1!
            uint8_t expectedREJ = (Ns == 0) ? REJ0 : REJ1;
            current_state = updateSupervisionFrame(byte, current_state, true);
            
            if(current_state == STOP)
            {
                if(expectedRR == received_control_byte)
                {
                    Ns = 1 - Ns;
                    alarm(0);//reset alarm! 
                    return frameLen;
               }
               else if(received_control_byte == expectedREJ ) 
               {
                alarm(0);
                alarmEnabled = FALSE;
                alarmCount++;//we should count right!?
                current_state = STATE_START;
               }
               else{
                current_state = STATE_START;
               }
            }
        } 
    }
    
    if(alarmCount >= 4) return -1; //Error message!

}

/*

» llclose
» sends DISC frame
» reads one byte at a time to receive DISC frame
» sends UA frame
» closes serial port
*/

/*
» llclose
» sends DISC frame
» reads one byte at a time to receive DISC frame
» sends UA frame
» closes serial port
*/
int llclose(int fd)
{
   
    unsigned char discFrame[5] = {
        FLAG,
        TRANSMITER,
        DISC,
        TRANSMITER ^ DISC,
        FLAG
    };

    alarmCount = 0;
    alarmEnabled = FALSE;
    current_state = STATE_START;
    int disc_received = 0; 
    while (alarmCount < 4 && current_state != STOP)
    {
        if (alarmEnabled == FALSE) {
            alarm(3);
            alarmEnabled = TRUE;

            write(fd, discFrame, 5);
            if (alarmCount > 0) { 
                printf("Timeout -> DISC frame resent (retry %d/3)\n", alarmCount);
            } else {
                printf("Sent DISC frame... waiting for Receiver's DISC.\n");
            }
        }

        uint8_t byte = 0;
        if(read(fd, &byte, 1) > 0)
        {
            current_state = updateSupervisionFrame(byte, current_state, true);
            
            if(current_state == STOP)
            {
                if(received_control_byte == DISC)
                {
                    alarm(0); 
                    disc_received = 1;
                }
                else
                {
                    current_state = STATE_START; 
                }
            } 
        } 
    }

    if (!disc_received) {
        printf("Error: Failed to receive DISC from receiver after 3 retries.\n");
    }

    
    unsigned char uaFrame[5] = {
        FLAG,
        TRANSMITER, 
        UA,                   
        TRANSMITER ^ UA,
        FLAG
    };

    write(fd, uaFrame, 5);
    printf("Sent final UA frame.\n");

    sleep(1); 

    
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    close(fd);
    printf("Serial port closed cleanly.\n");
    

    return (disc_received ? 1 : -1);
}

int setup_termios(int fd)
{
    struct termios newtio;

    // Save current port settings to the global oldtio
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        return -1;
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;

    // --- CRITICAL FIX FOR YOUR STATE MACHINE ---
    newtio.c_cc[VTIME] = 0; // Do not use the termios timer
    newtio.c_cc[VMIN] = 0;  // Block read() until exactly 0 byte arrives

    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    printf("New termios structure set\n");
    return 0;
}


#endif