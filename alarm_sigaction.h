#ifndef ALARM_SIGACTION_H
#define ALARM_SIGACTION_H

// 1. Define your macros
#define FALSE 0
#define TRUE 1

// 2. Use 'extern' to tell other files that these variables exist somewhere else
extern int alarmEnabled;
extern int alarmCount;

// 3. Declare your function prototypes (just the names, no logic)
void alarmHandler(int signal);
int setup();

#endif // ALARM_SIGACTION_H