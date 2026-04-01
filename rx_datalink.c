#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "stateMachine.h"

#define MAX_SIZE 256 //Bytes

#define FLAG 0x7E

//Alarm params
#define TIMEOUT_RECEIVER 5 // seconds
#define TIMEOUT_TRANSMITER 3 //Seconds

#define MAX_ALARM_COUNT_RX 4 



int alarmEnabled = FALSE;
int alarmCount  = 0;

CONTROL frame_number_to_receive; //global

/**
 * @return 
 * \n -1 -> could not open serial port 
 * \n -2 -> could not tcgetattr 
 * \n -3 -> could not set tcsetattr
 * \n -4 -> could not set alarm (sigaction)
 * \n -5 -> timeout waiting for set frame
 */
int llopen(char portName[], bool isTransmitter){
    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        return -1;
    }

    
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        return -2;
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return (-3);
    }
    printf("New termios structure set\n");


    // Set alarm function handler.
    // Install the function signal to be automatically invoked when the timer expires,
    // invoking in its turn the user function alarmHandler
    struct sigaction act = {0};
    act.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
        perror("sigaction");
        return (-4);
    }
    printf("Alarm configured\n");

    if (isTransmitter == 0)
    {
       //receiver stuff
        if(wait_set_frame(fd) < 0){
            return -5;
        }
        //set frame received
        send_ua(fd);
    }//falta trasmitter
    
    frame_number_to_receive = RR0; //imporatante!!
    
    return fd;
}

/**
 * @return 
 * \n 0 -> set reeived
 * \n -1 -> alarm count > MAX_ALARM_COUNT_RX
 */
int wait_set_frame(int fd){
    alarmCount = 0; //global variable
    alarmEnabled = 0; //global variable
    STATE currentState = STATE_START;
    while (1)
    {
        if (alarmEnabled == FALSE)
        {   
            printf("Setting up receiver alarm, with alarm count of %d...", alarmCount);
            alarm(TIMEOUT_RECEIVER); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }

        char byte;
        int bytesRead = read(fd, &byte, 1);
        if(bytesRead > 0){
            //update state machine
            updateSupervisionFrame(byte, &currentState, 1);
        }
        if(alarmCount > MAX_ALARM_COUNT_RX){
            alarm(0);
            return -1;
        }
        if (currentState == STOP){
            alarm(0);
            return 0;
        }
    }
}

/**
 * @return 
 * \n -1 -> not able to sent all
 * \n 0 -> ok
 */
int send_ua(int fd){
    alarmCount = 0; //global variable
    alarmEnabled = 0; //global variable
    STATE currentState = STATE_START;
    char buf[5]; //UA Frame
    buf[0]= FLAG;
    buf[1]= 0x03;
    buf[2]= 0x03;
    buf[3]= buf[1]^buf[2];
    buf[4]= FLAG;

    //Trying to send
    int bytes = write(fd, buf, 5);
    printf("%d bytes written\n", bytes);
    
    if(bytes != 5){
        return -1;
    }
    //PROBLEM -> NOT RETRY LOGGIC
    //Não sei ainda como vou fazer
    //pq ler I frame em si tem de estar no llread,
    //então terei de receber o número de bytes lidos pelo llread e se há erro no primeiro para reenviar o UA frame
    return 0;
}

/**
 * Pelo que percebi, esta função lê uma frame I apenas, e retorna para a application layer. A application layer é que tem de ir
 * chamando várias vezes esta função para isso acontecer
 * @return 
 * \n -1 -> buffer demasiado pequeno
 * \n -2 -> max retries excedida
 * \n NUMBER OF BYTES READ -> sucesso
 */

int llread(int fd, char* buf){
    uint16_t size_buf = sizeof(buf);

    char bufSend[5];

    bufSend[0] = FLAG;
    bufSend[1] = 0x03;
    bufSend[2] = 0; //will be set in the loop
    bufSend[3] = 0; //same
    bufSend[4] = FLAG;

    STATE currentState = STATE_START;   

    uint64_t bufCounter = 0;
    uint8_t BCC2_tracker = 0;

    alarmEnabled = 1; //global variable -> tem de começar a 1!!!!
    alarm(TIMEOUT_RECEIVER); //Eu faço isto assim que é para a lógica do loop não ter de ter uma exceção para o primeiro caso
    alarmCount = 0; //global variable
    
    int error_msg = 0;
    while (1)
    {
        if (alarmEnabled == FALSE)
        {   
            //Request the frame que ainda não recebeu
            
            bufSend[3] = bufSend[1]^bufSend[2]; //BCC1

            int bytes = write(fd, bufSend, 5);
            printf("Supervision frame sent. %d bytes written\n", bytes);
            printf("Frame number waiting to receive: %d\n", frame_number_to_receive);

            alarm(TIMEOUT_RECEIVER);
            alarmEnabled = TRUE;
        }

        if(alarmCount > MAX_ALARM_COUNT_RX){
            error_msg = -2;
            break;
        }


        char byte;
        int bytesRead = read(fd, &byte, 1);
        if (bytesRead == 0)
            continue;
        
        //update state machine
        received_control_byte = 0;
        currentState = updateIFrame(byte, currentState);
        //received_control_byte é atualizado dps da função

        if (currentState == BCC_OK){

            if(bufCounter >= size_buf || bufCounter >= MAX_SIZE){
                error_msg =  -4; //maior do que o buffer predefinido
                break;
            }
            if((received_control_byte == frame_number_to_receive)){
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

            //Acrescentar byte ao buffer
            buf[bufCounter] = byte;
            bufCounter++;
        }

        if (currentState == STOP){
            //Verificar BCC2 dos dados
            if (buf[bufCounter - 1] == BCC2_tracker)
            {
                frame_number_to_receive == RR0 ? RR1 : RR0;
                bufCounter--; //BCC2 não é uma "data"
                error_msg = bufCounter; //está à trolha mas é só pra poder retornar este valor
                break;
            }else{
                bufSend[2] = frame_number_to_receive == RR0 ? REJ0 : REJ1; //receber o mesmo, porque não o conseguiu ler

                int bytes = write(fd, bufSend, 5);
                printf("REJ frame sent. %d bytes written\n", bytes);
                printf("Frame number waiting to receive: %d\n", frame_number_to_receive);
            }
            
        }

        
    }

    alarm(0);
    return error_msg
}



 //Não é preciso fazer distinção entre receiver e transmitter?
 //Porque um espera pelo DISC antes de mandar
 /**
  * @return 
  * 0 ok -> closed with success
  * -1 -> sent DISC but did not receive DISC
  * -3 -> error writing UA
  */
int llclose(int fd, bool isTransmitter){
    alarmEnabled = 0; //global variable
    alarmCount = 0; //global variable

    char bufDISC[5];

    bufDISC[0] = FLAG;
    bufDISC[1] = 0x03;
    bufDISC[2] = 0x0B;
    bufDISC[3] = bufDISC[1] ^ bufDISC[2];
    bufDISC[4] = FLAG;

    STATE currentState = STATE_START;

    int error_msg = 0;
    if (isTransmitter)
    {    
        while (1)
        {
            if (alarmEnabled == FALSE)
            {   
                int bytes = write(fd, bufDISC, 5);
                printf("DISC frame sent. %d bytes written\n", bytes);

                alarm(TIMEOUT_RECEIVER); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;
            }

            char byte;
            int bytesRead = read(fd, &byte, 1);
            if(bytesRead > 0){
                //byte
                //update state machine
                received_control_byte = 0;
                updateSupervisionFrame(byte, currentState, 1); //ESTA FUNÇÃO VAI SER BUÉ GERAL E LEVAR COM A E C COMO 
                //neste caso, do DISC
            }
            if (currentState == STOP){
                error_msg = 0;
                break;
            }
            if(alarmCount > MAX_ALARM_COUNT_RX){
                error_msg = -1;
                break;
            }
        }
        alarm(0);
        if(error_msg < 0)
            return error_msg;
        
        //enviou DISC e recebeu DISC
        if(send_ua(fd) < 0){
            return -3;
        }
        return 0;
    }else{
        while (1)
        {
            if (alarmEnabled == FALSE)
            {   
                alarm(TIMEOUT_RECEIVER);
                alarmEnabled = TRUE;
            }

            char byte;
            int bytesRead = read(fd, &byte, 1);
            if(bytesRead > 0){
                //byte
                //update state machine
                updateSupervisionFrame(byte, currentState, 1); //ESTA FUNÇÃO VAI SER BUÉ GERAL E LEVAR COM A E C COMO 
                //neste caso, do DISC
            }
            if (currentState == STOP){
                if(received_control_byte == DISC){
                    error_msg = 0;
                    break;
                }
                currentState = STATE_START;
            }
            if(alarmCount > MAX_ALARM_COUNT_RX){
                error_msg = -1;
                break;
            }
        }
        //recebeu DISC
        if(error_msg < 0)
            return error_msg;
        
        //send DISC
        int bytes = write(fd, bufDISC, 5);
        printf("DISC frame sent. %d bytes written\n", bytes);


        //receber UA
        alarmEnabled = 0;
        while (1)
        {
            if (alarmEnabled == FALSE)
            {   
                alarm(TIMEOUT_RECEIVER); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;
            }

            char byte;
            int bytesRead = read(fd, &byte, 1);
            if(bytesRead > 0){
                //byte
                //update state machine
                updateSupervisionFrame(byte, &currentState, 1); //ESTA FUNÇÃO VAI SER BUÉ GERAL E LEVAR COM A E C COMO 
                //neste caso, do DISC
            }
            if (currentState == STOP){
                error_msg = 0;
                break;
            }
            if(alarmCount > MAX_ALARM_COUNT_RX){
                error_msg = -1;
                break;
            }
        }
        alarm(0);
        //recebeu UA
        return error_msg;
    }

}