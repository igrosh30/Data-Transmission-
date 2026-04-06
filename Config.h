#ifndef CONFIG_H
#define CONFIG_H
#include <unistd.h>
#include <stdint.h>

#define FLAG 0x7E
#define TRANSMITER 0x03 //either command or response
#define RECEIVER 0x01 //either command or response


typedef struct {
    uint8_t flag;
    uint8_t adress;
    uint8_t control;
    uint8_t bcc1;
    uint8_t frameReady;
}Frame;

typedef enum{
    SET = 0x03,
    UA  = 0x07,
    RR0 = 0x05,
    RR1 = 0x85,
    REJ0= 0x01,
    REJ1= 0x81,
    DISC= 0x0B,
    IF0= 0X00,
    IF1= 0X40
}CONTROL;



typedef struct {
    uint8_t baudrate;
    uint8_t timeout;
    uint8_t numTries;
} DLLConfig;


/*
• A (Address) to distinguish whether the frame is a command or a reply
• C (Control) to distinguish between the different types of supervision
frames and to carry sequence numbers N(s) in I frames
• BCC (Block Check Character), error detection mechanism
*/

#endif
