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

    if (setup_termios(fd) == -1) {
        return -1; 
    }

    
    unsigned char setFrame[5] = {
        FLAG,
        TRANSMITER,
        0x03,                   
        TRANSMITER ^ 0x03,
        FLAG
    };

    int bytes = write(fd, setFrame, 5);
    printf("%d bytes written\n", bytes);

    printf("Sent data... waiting\n");

    setup();//setup the alarm! 

    unsigned char bufR[MAX_SIZE] = {0};
    
    
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
                alarm(0);//reset alarm! 
                return fd;//all correct! 
            } 
        } 
    }
    // === FAILED ===
    printf("Failed to receive UA after %d retries\n", alarmCount);
    close(fd);
    return -1;  
}


/*
 llwrite
» implements error detection (compute BCC over the data packet)
» parses data packet to implement byte stuffing (transparency)
» builds Information frame Ns (Ns=0 or 1)
» sends I frame
» reads one byte at a time to receive response
» if negative response (REJ) or no response, resends I frame up to a maximum number of times
(retransmission mechanism explained in slide 46)
» return success or failure
*/
int llwrite(int fd, const unsigned char *buf, int bufSize)
{
    //write in fd
    if (fd < 0 || buf == NULL || bufSize <= 0) return -1;
    static int Ns = 0;

    uint8_t bcc2 = 0;
    for (int i = 0; i < bufSize; i++) {
        bcc2 ^= buf[i];
    }

    unsigned char stuffed[MAX_SIZE * 2];//can we set to 256*2 !??
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

    //create a mehtod to buil the header!? 
    unsigned char control= Ns ? 0x00 : 0x40;
    unsigned char headerFrame_TX[4] = {
        FLAG,
        TRANSMITER,//Adress field
        control,                  
        TRANSMITER ^ control,//BBC1
    };
    //write the header:
    int header_bytes = wirte(fd,headerFrame_TX,4); // without the flag! 
    int payload_bytes = wirte(fd, stuffed, stuffedLen); // wirte data bytes
    int last_bytes = write (fd, [bcc2,FLAG], 2);//have error! 

    //what we should receive? ...
    unsigned char bufR[MAX_SIZE] = {0};
    
    
    while (alarmCount < 4 && current_state != STOP)
    {
        if (alarmEnabled == FALSE) {
            alarm(3);
            alarmEnabled = TRUE;

            if (alarmCount > 0) { 
                //write again the frame to rx read! 
                int header_bytes = wirte(fd,headerFrame_TX,4); // without the flag! 
                int payload_bytes = wirte(fd, stuffed, stuffedLen); // wirte data bytes
                int last_bytes = write (fd, [bcc2,FLAG], 2);//have error! 
                printf("Timeout → SET frame resent (retry %d/3)\n", alarmCount);
            }
        }

        //Do the reading:
        uint8_t byte = 0;
        if(read(fd, &byte, 1) > 0)
        {
            current_state = updateSupervisionFrame(byte, current_state, true);
            //
            if(current_state == STOP)
            {
                alarm(0);//reset alarm! 
                //Check what we received! 
                return (header_bytes + payload_bytes + last_bytes);
                //return all the bytes written or just the payload ones??
            } 
        } 
    }
    
    if(alarmCount == 3) return -1; //Error message!

}

/*

» llclose
» sends DISC frame
» reads one byte at a time to receive DISC frame
» sends UA frame
» closes serial port
*/


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