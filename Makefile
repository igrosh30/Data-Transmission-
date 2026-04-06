# No flags at all
CC = gcc

# The final program name
TARGET = main

# Your project files
SRCS = main.c dll.c stateMachine.c rx_appLayer.c tx_appLayer.c alarm_sigaction.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)