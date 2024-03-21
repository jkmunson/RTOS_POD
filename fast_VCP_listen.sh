#!/bin/bash
stty -echo -F /dev/ttyACM0
gcc baud_set.c -o baud_set >/dev/null 2>&1
./baud_set /dev/ttyACM0 16000000 >/dev/null
cat /dev/ttyACM0
