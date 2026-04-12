#!/bin/bash

# Define the 10 baud rates you want to test
BAUDRATES="9600 19200 38400 57600 115200 128000 230400 256000 460800 500000"

for baud in $BAUDRATES; do
    echo ">>> Preparing for Baud Rate: $baud"
    
    # We need to run the receiver 10 times for this specific baud rate
    for i in {1..10}; do
        echo "  [Run $i/10] Waiting for data..."
        ./main /dev/ttyS0 0 received_penguin.gif $baud
    done
    
    echo ">>> Finished all runs for $baud. Moving to next speed."
    echo "------------------------------------------------"
done