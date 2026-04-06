# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Target executable name
TARGET = main

# Source files
SRCS = main.c dll.c stateMachine.c rx_appLayer.c tx_appLayer.c alarm_sigaction.c

# Object files (automatically replaces .c with .o)
OBJS = $(SRCS:.c=.o)

# Default rule: build the executable
all: $(TARGET)

# Linking the object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compiling individual source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule to remove temporary files and the executable
clean:
	rm -f $(OBJS) $(TARGET)