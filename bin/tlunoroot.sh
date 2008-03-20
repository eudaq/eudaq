#!/bin/bash

USBINF=`lsusb -d 0x165d:0x0001`
if [ "$USBINF" == "" ]; then
  echo "No TLU Detected"
  exit 1
fi

USBBUS=`echo $USBINF | sed 's/^Bus \([0-9]*\) Device \([0-9]*\).*/\1/'`
USBDEV=`echo $USBINF | sed 's/^Bus \([0-9]*\) Device \([0-9]*\).*/\2/'`

echo "Found TLU on Bus $USBBUS, device $USBDEV, making it accessible as non-root"
echo "You may be prompted for the password here..."

sudo chmod a+w /proc/bus/usb/$USBBUS/$USBDEV

echo "Done."
