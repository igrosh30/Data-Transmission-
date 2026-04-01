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


/*
• A (Address) to distinguish whether the frame is a command or a reply
• C (Control) to distinguish between the different types of supervision
frames and to carry sequence numbers N(s) in I frames
• BCC (Block Check Character), error detection mechanism
*/

#endif