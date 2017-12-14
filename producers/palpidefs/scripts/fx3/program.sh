#!/bin/bash

# enter the folder where this script is in
cd "$( dirname "${BASH_SOURCE[0]}" )"

N=`lsusb | grep "04b4:00f3 Cypress Semiconductor Corp" | wc -l`
while [ "$N" -gt 0 ]
do
    echo "$N unprogrammed FX3 found. Trying to program them all..."

    I=0
    while [ "$I" -lt "$N" ]
    do
        echo "Programming..."
        ./download_fx3 -t RAM -i SlaveFifoSync.img
        if [ "$?" -eq 0 ]
        then
            let I=I+1
        fi
        sleep 1
    done
    N=`lsusb | grep "04b4:00f3 Cypress Semiconductor Corp" | wc -l`
done
