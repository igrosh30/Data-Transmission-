#!/bin/bash

# Must match the Receiver's list exactly!
BAUDRATES="9600 19200 38400 57600 115200 128000 230400 256000 460800 500000"

for baud in $BAUDRATES; do
    echo ">>> Starting tests for Baud Rate: $baud"
    
    # Run the transmitter 10 times
    for i in {1..10}; do
        echo "  [Run $i/10] Sending file..."
        # Note: we pass '1' for numRuns so the C code does 1 transfer and exits
        ./main /dev/ttyUSB0 1 penguin.gif $baud 1 0.0
        
        # Small sleep to let the Receiver computer save the file and restart
        sleep 2
    done
    
    echo ">>> Completed $baud. Check results.csv."
    echo "------------------------------------------------"
    sleep 5 # Time for you to read the screen before the next baud rate
done