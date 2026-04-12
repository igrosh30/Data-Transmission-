Transimter runing multiple times:
for baud in 38400 57600 115200; do
    echo "=== Testing baudrate $baud with 8 runs ==="
    ./main /dev/ttyUSB0 1 penguin.gif $baud 8 0.0
    sleep 3
done
At the receiver:
./transmitter /dev/ttyS0 0 received_penguin.gif
