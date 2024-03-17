#!/bin/bash
sudo stty -parenb -F /dev/ttyACM0 16000000 cs8 -cstopb raw
sudo cat /dev/ttyACM0
