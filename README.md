Transimter runing multiple times:
for baud in 38400 57600 115200; do
    echo "=== Testing baudrate $baud with 8 runs ==="
    ./main /dev/ttyUSB0 1 penguin.gif $baud 8 0.0
    sleep 3
done
At the receiver:
./transmitter /dev/ttyS0 0 received_penguin.gif

etedu@tux12:~/Desktop/Data-Transmission-$ ./main /dev/ttyS0 0 received_pinguin.gif 115200
=== RECEIVER - baud 115200 ===

Port: /dev/ttyS0 | Output file: received_pinguin.gif | baud=115200 | tries=3 | timeout=3
llopnen
        Config provided:
                Baudrate: 115200
                Timeout: 3
                NumTries: 3
        Serial port opned.
        New termios structure set
        Termios setup successfully
        Alarm setup successfully
        Timeout -> Frame resent (retry 0/3)
Alarm #1 received
        Timeout -> Frame resent (retry 1/3)
Alarm #2 received
        Timeout -> Frame resent (retry 2/3)
Alarm #3 received
        Timeout -> Frame resent (retry 3/3)
Alarm #4 received
                Error waiting for SET frame
ERROR: llopen failed (code -1)
=== File transfer finished (receiver) ===


chmod +x rx.sh
./rx.sh