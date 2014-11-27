#!/bin/bash

# enter the folder where this script is in
cd "$( dirname "${BASH_SOURCE[0]}" )"

N=`lsusb | grep "04b4:00f3 Cypress Semiconductor Corp" | wc -l`

echo "$N unprogrammed FX3 found. Trying to program them all..."

I=0
while [ "$I" -lt "$N" ]
do
  echo "Programming..."
  ./download_fx3 -t RAM -i SlaveFifoSync.img
  sleep 1
  let I=I+1
done
