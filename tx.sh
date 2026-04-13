#!/bin/bash

# Must match the Receiver's list exactly!
BAUDRATES="9600 19200 38400 57600 115200 128000 230400 256000 460800 500000"
DATA_SIZES="50 100 200 300 400 500 600 700 800"
FER_VALUES="0.0 0.01 0.05 0.1"

for baud in $BAUDRATES; do
    echo "================================================"
    echo ">>> Baud Rate: $baud"
    echo "================================================"
    
    for size in $DATA_SIZES; do
        
        for fer in $FER_VALUES; do
            echo "  >>> Testing Size: $size bytes | FER: $fer"
            
            # Run the transmitter 10 times for each combination
            for i in {1..10}; do
                echo "  [Run $i/10] Sending..."
                
                # Args: <port> 1 <file> <baud> <numRuns> <fer> <dataSize>
                ./main /dev/ttyUSB0 1 penguin.gif $baud 1 $fer $size
                
                sleep 2
            done
        done
        
    done
    sleep 5
done