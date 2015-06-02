#!/bin/bash

# enter the folder where this script is in
cd "$( dirname "${BASH_SOURCE[0]}" )"

# compile the programming code
g++ -shared -fPIC libcyusb.c -o libcyusb.0.1.so -l usb-1.0

g++ -c -o libcyusb.o libcyusb.c
ar rcs libcyusb.a libcyusb.o

g++ -o download_fx3             download_fx3.c              -lcyusb -L. -l usb-1.0

