#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "Config.h"
#include "alarm_sigaction.h" 
#include "stateMachine.h"


#include "dll.h"

#define DESCRIPTION 1
#define DEBUG 0

struct linkLayer linkLayer;

struct termios oldtio;

uint8_t frame_number_to_receive;

/**
 * llopen
 * opens serial port (invokes openSerialPort)
 * sends SET frame
 * reads one byte at a time to receive UA frame (see state machine in slide 40)
 * returns success or failure
 * @return
 * \n -1 -> Alarm count exceeded
 * \n -2 -> error writing frame
 * \n -5 -> error opening serial port
 * \n -6 -> error setting termios
 * \n -7 -> error setting alarm
 * \n -8 -> no config provided
 */
int llopen(const char serialPortName[], bool isTransmitter, DLLConfig *config)
{
    if (DESCRIPTION)
    {
        printf("llopnen\n");
    }
    
    //Verifications on config values
    if(config == NULL){
        printf("\tNo config provided, using default values.\n");
        return -8; //no config provided
    }
    
    if (DESCRIPTION)
    {
        printf("\tConfig provided:\n");
        printf("\t\tBaudrate: %d\n", config->baudrate);
        printf("\t\tTimeout: %d\n", config->timeout);
        printf("\t\tNumTries: %d\n", config->numTries);
    }
    linkLayer.baudRate = config->baudrate;
    linkLayer.timeout = config->timeout;
    linkLayer.numTransmissions = config->numTries;



    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        printf("\t\tError opening Serial port\n");
        return -5;
    }
    if (DESCRIPTION)
    {
        printf("\tSerial port opned.\n");
    }

    if (setup_termios(fd) == -1)
    {
        printf("\t\tError setup termios\n");
        return -6;    
    }
    if(DESCRIPTION){
        printf("\tTermios setup successfully\n");
    }
    
    if(setup() == -1){
        printf("\t\tError setup alarm\n");
        return -7;
    }
    if(DESCRIPTION){
        printf("\tAlarm setup successfully\n");
    }

    if(isTransmitter){
        int error = send_C_N_wait_C(fd, SET, UA);
        if (error < 0)
        {
            printf("\t\tError sending SET and waiting for UA\n");
            return error;
        }
        if (DESCRIPTION)
        {
            printf("\tSET sent and UA received successfully\n");
        }
        
    }else{
        int error = wait_C(fd, SET);
        if (error < 0)
        {
            printf("\t\tError waiting for SET frame\n");
            return error;
        }

        if(DESCRIPTION)
        {
            printf("\tSET frame received successfully\n");
        }

        error = send_C(fd, UA);
        if (error < 0)
        {
            printf("\t\tError sending UA frame\n");
            return error;
        }

        if (DESCRIPTION)
        {
            printf("\tUA frame sent successfully\n");
        }
        
    }

    printf("llopen completed successfully\n\n");
    frame_number_to_receive = RR0; //inicializar esta variável no início
    return fd;  
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
    printf("llwrite\n");
    printf("\tfd : %d\nbuf : %p \nbufSiez: %d\n", fd, buf, bufSize);
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
    STATE current_state = STATE_START;
    while (alarmCount < 4 && current_state != STOP)
    {
        if (alarmEnabled == FALSE) {
            alarm(3);
            alarmEnabled = TRUE;

            if (alarmCount > 0) { 
                //C    = (Ns == 0) ? 0x00 : 0x40; //Alex tentou alterar 
                write(fd, I_frame, frameLen);
            }
        }

        //Do the reading:
        uint8_t byte = 0;
        
        if(read(fd, &byte, 1) > 0)
        {
            printf("Bytes Received: 0x%x\n", byte);
            uint8_t expectedRR = (Ns == 0) ? RR1 : RR0;//receiver got message 0 - send ready for 1!
            uint8_t expectedREJ = (Ns == 0) ? REJ0 : REJ1;
            current_state = updateSupervisionFrame(byte, current_state, true);
            printf("\tcurrent_state: %d\n", current_state);
            
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


/**
 * Pelo que percebi, esta função lê uma frame I apenas, e retorna para a application layer. A application layer é que tem de ir
 * chamando várias vezes esta função para isso acontecer
 * @return 
 * \n -1 -> buffer demasiado pequeno
 * \n -2 -> max retries excedida
 * \n -4 -> buffer predefinido excedido
 * \n NUMBER OF BYTES READ -> sucesso
 */

int llread(int fd, char* buf, uint16_t size_buf){
    printf("llread\n");

    char bufSend[5];

    bufSend[0] = FLAG;
    bufSend[1] = 0x03;
    bufSend[2] = 0; //will be set in the loop
    bufSend[3] = 0; //same
    bufSend[4] = FLAG;

    STATE current_state = STATE_START;

    uint64_t bufCounter = 0;
    uint8_t BCC2_tracker = 0;

    alarmEnabled = 1; //global variable -> tem de começar a 1!!!!
    alarm(linkLayer.timeout); //Eu faço isto assim que é para a lógica do loop não ter de ter uma exceção para o primeiro caso
    alarmCount = 0; //global variable
    
    int error_msg = 0;
    
    received_control_byte = 0;
    while (1)
    {
        if (alarmEnabled == FALSE)
        {   
            //Request the frame que ainda não recebeu
            //Assume-se que algures neste ciclo, bufSend (aka Control flag) é atualizada
            
            bufSend[3] = bufSend[1]^bufSend[2]; //BCC1

            int bytes_written = write(fd, bufSend, 5);

            if(bytes_written != 5){
                printf("\tError writing frame to serial port\n");
                error_msg = -2; // Error writing frame
                break;
            }

            if(DESCRIPTION){
                printf("\tTimeout -> Frame resent (retry %d/%d)\n", alarmCount, linkLayer.timeout);
            }

            alarm(linkLayer.timeout);
            alarmEnabled = TRUE;
            current_state = STATE_START;
            received_control_byte = 0;
        }

        if(alarmCount > MAX_ALARM_COUNT_RX){
            error_msg = -1; //max retries exceeded
            break;
        }

        //read 
        char byte;
        int bytesRead = read(fd, &byte, 1);
        if (bytesRead == 0)
            continue;
        
        //update state machine
        current_state = updateIFrame(byte, current_state);
        if(DEBUG){
                printf("\tByte read: 0x%x\n", byte);
                printf("\tCurrent state: %d\n", current_state);
        }
        //received_control_byte é atualizado dps da função
        printf("received_control_byte: %x\n", received_control_byte);
        printf("current state: %d\n", current_state);

        if (current_state == DATA){
            //destuffing e guardar no buffer
            if(byte == 0x5e && buf[bufCounter - 1] == 0x7d) //flag destuffed
            {
                //0x7e 
                bufCounter--; //remover o 0x7d do buffer
                BCC2_tracker = BCC2_tracker ^ 0x7d; //remover o efeito do 0x7d no BCC2 
                byte = 0x7e;
            }else if(byte == 0x5d && buf[bufCounter - 1] == 0x7d) //escape destuffed
            {
                //0x7d
                bufCounter--; //remover o 0x7d do buffer
                BCC2_tracker = BCC2_tracker ^ 0x7d; //remover o efeito do 0x7d no BCC2
                byte = 0x7d;
            }

            if(
                bufCounter >= size_buf - 1 || //como eu neste momento estou a guardar o BCC antes de o rejeitar, tenho de garantir isto
                bufCounter >= MAX_SIZE
            )
            {
                printf("\tBc: %d\nsize_buf: %d\n MAX: %d", bufCounter, size_buf, MAX_SIZE);
                error_msg = -4; //maior do que o buffer predefinido
                break;
            }

            uint8_t byte_to_receive = frame_number_to_receive == RR0 ? IF0 : IF1;
            if(received_control_byte != byte_to_receive){
                printf("\tI Frame received out of order\n");
                printf("\t\tExpected control byte: 0x%x\n", byte_to_receive);
                printf("\t\tReceived control byte: 0x%x\n", received_control_byte);
                //está a receber duplicado
                bufSend[2] = frame_number_to_receive; //send RRx
                alarmEnabled = 0; //vai voltar a pedir aquele frame que ele queria
                alarm(0);
                continue;
            }
            //recebeu o correto!

            //Atualizar BCC2
            if (bufCounter == 0) //ou seja, primeiro valor 
            {
                BCC2_tracker = byte;
            }else{
                BCC2_tracker = BCC2_tracker ^ byte;
            }
            if (DEBUG)
            {
                printf("\tBCC2_Tracker: 0x%x\n", BCC2_tracker); 
            }

            //Acrescentar byte ao buffer
            buf[bufCounter] = byte;
            bufCounter++;
        }

        if (current_state == STOP){
            //Verificar BCC2 dos dados
            //Qnd faço xxxx^xxxx dá sempre 0. Logo se BCC2_tracker for 0, então está correto
            if (BCC2_tracker == 0)
            {
                
                
                frame_number_to_receive = frame_number_to_receive == RR0 ? RR1 : RR0;
                bufCounter--; //BCC2 não é uma "data"
                error_msg = bufCounter; //está à trolha mas é só pra poder retornar este valor


                bufSend[2] = frame_number_to_receive; //send RRx
                bufSend[3] = bufSend[1]^bufSend[2];
                int bytes = write(fd, bufSend, 5);
                if(bytes != 5){
                    printf("\tError writing frame to serial port\n");
                    error_msg = -2; // Error writing frame
                    break;
                }
                printf("\tRR frame sent. %d bytes written\n", bytes);
                printf("\tFrame number waiting to receive: %d\n", frame_number_to_receive == RR0 ? 0 : 1);

                break;
            }else{
                bufSend[2] = frame_number_to_receive; //receber o mesmo, porque não o conseguiu ler

                int bytes = write(fd, bufSend, 5);
                if(bytes != 5){
                    printf("\tError writing frame to serial port\n");
                    error_msg = -2; // Error writing frame
                    break;
                }
                printf("\tREJ frame sent. %d bytes written\n", bytes);
                printf("\tFrame number waiting to receive: %d\n", frame_number_to_receive);
            }
            
        }

        
    }

    alarm(0);
    return error_msg;
}


/**
 * llclose
 * IF Transmitter:
 *   sends DISC frame
 *  reads one byte at a time to receive DISC frame
 *  sends UA frame
 * IF Receiver:
 *  reads one byte at a time to receive DISC frame
 *  sends DISC frame
 *  reads one byte at a time to receive UA frame
 * closes serial port
 * @return
 * \n -1 -> timeout (max alarm count exceeded)
 * \n -2 -> error writing frame
 * \n 0 -> success
 */
int llclose(int fd, bool isTransmitter)
{
    printf("llclose\n");
   
    if(isTransmitter){
        int er = send_C_N_wait_C(fd, DISC, DISC);
        if (er < 0) {
            return er;
        }

        if(DESCRIPTION){
            printf("\tDISC sent and received successfully\n");
        }

        int er2 = send_C(fd, UA);
        if (er2 < 0) {
            return er2;
        }
        if(DESCRIPTION){
            printf("\tUA sent successfully\n");
        }
    }else{
        int er = wait_C(fd, DISC);
        if (er < 0) {
            return er;
        }

        int er2 = send_C(fd, DISC);
        if (er2 < 0) {
            return er2;
        }

        int er3 = wait_C(fd, UA);
        if (er3 < 0) {
            return er3;
        }
        
    }

    sleep(1); 

    
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    close(fd);
    printf("Serial port closed cleanly.\n");
    
    return 0;

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

    printf("\tNew termios structure set\n");
    return 0;
}

/**
 * send_C_N_wait_C
 * @return
 * \n -1 -> timeout (max alarm count exceeded)
 * \n -2 -> error writing frame
 * \n 0 -> success
 */
int send_C_N_wait_C(int fd, unsigned char C_send, unsigned char C_receive){
    unsigned char sendFrame[5] = {
        FLAG,
        TRANSMITER,
        C_send,
        TRANSMITER ^ C_send,
        FLAG
    };

    alarmCount = 0;
    alarmEnabled = FALSE;
    STATE current_state = STATE_START;
    int error = 0;
    while (1)
    {
        if (alarmEnabled == FALSE) {
            alarm(linkLayer.timeout);
            alarmEnabled = TRUE;

            int bytes_written = write(fd, sendFrame, 5);
            if (bytes_written != 5) {
                printf("\tError writing frame to serial port\n");
                error = -2; // Error writing frame
                break;
            }
            if (alarmCount > 0) { 
                printf("\tTimeout -> Frame resent (retry %d/%d)\n", alarmCount, linkLayer.timeout);
            } else {
                printf("\tSent frame... waiting for Receiver's response.\n");
            }
        }

        uint8_t byte = 0;
        if(read(fd, &byte, 1) > 0)
        {
            current_state = updateSupervisionFrame(byte, current_state, true);

            if (DEBUG)
            {
                printf("\tByte: 0x%x\n", byte);
                printf("\tCurrent state: %d\n", current_state);
            }
            
            if(current_state == STOP)
            {
                if(received_control_byte == C_receive)
                {
                    //error = 0;
                    break;
                }
                else
                {
                    current_state = STATE_START; 
                }
            } 
        }
        if(alarmCount >= MAX_ALARM_COUNT_RX){
            error = -1;
            break;
        }
    }
    alarm(0);

    return error;
}


/**
 * wait_C
 * @return
 * \n -1 -> timeout (max alarm count exceeded)
 * \n 0 -> success
 */
int wait_C(int fd, unsigned char C_receive){
    alarmCount = 0;
    alarmEnabled = FALSE;
    STATE current_state = STATE_START;
    int error = 0;
    while (1)
    {
        if (alarmEnabled == FALSE) {
            alarm(linkLayer.timeout);
            alarmEnabled = TRUE;

            printf("\tTimeout -> Frame resent (retry %d/%d)\n", alarmCount, linkLayer.timeout);

        }

        uint8_t byte = 0;
        if(read(fd, &byte, 1) > 0)
        {
            current_state = updateSupervisionFrame(byte, current_state, true);

            if (DEBUG)
            {
                printf("\tByte: 0x%x\n", byte);
                printf("\tCurrent state: %d\n", current_state);
            }
            
            if(current_state == STOP)
            {
                if(received_control_byte == C_receive)
                {
                    //error = 0;
                    break;
                }
                else
                {
                    current_state = STATE_START; 
                }
            } 
        }
        if(alarmCount >= MAX_ALARM_COUNT_RX){
            error = -1;
            break;
        }
    }
    alarm(0);

    return error;
}

/**
 * send_C
 * @return
 * \n -2 -> error writing frame
 * \n 0 -> success
 */
int send_C(int fd, unsigned char C_send){
    unsigned char sendFrame[5] = {
        FLAG,
        TRANSMITER,
        C_send,
        TRANSMITER ^ C_send,
        FLAG
    };
    int bytes_written = write(fd, sendFrame, 5);
    if (bytes_written != 5) {
        printf("\t\tError writing frame to serial port\n");
        return -2; // Error writing frame
    }
    return bytes_written;
}
