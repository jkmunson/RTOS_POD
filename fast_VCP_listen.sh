#!/bin/bash
<<<<<<< HEAD
gcc baud_set.c -o baud_set 2>1 >/dev/null
=======
gcc baud_set.c -o baud_set 2>&1 >/dev/null
>>>>>>> cec6eac (Wrote a program for setting the baud rate on linux to an arbitrary value -> Got the debug UART VCP running at the full 16Mbaud.)
./baud_set /dev/ttyACM0 16000000 >/dev/null
cat /dev/ttyACM0
