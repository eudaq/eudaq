#!/bin/bash

if which lsusb &> /dev/null; then
  USBINF=`lsusb -d 0x165d:0x0001`
elif which /sbin/lsusb &> /dev/null; then
  USBINF=`/sbin/lsusb -d 0x165d:0x0001`
else
  echo "No lsusb command detected, please install usbutils"
  exit 1
fi

if [ "$USBINF" == "" ]; then
  echo "No TLU Detected"
  exit 1
fi

USBBUS=`echo $USBINF | sed 's/^Bus \([0-9]*\) Device \([0-9]*\).*/\1/'`
USBDEV=`echo $USBINF | sed 's/^Bus \([0-9]*\) Device \([0-9]*\).*/\2/'`

echo "Found TLU on Bus $USBBUS, device $USBDEV, making it accessible as non-root"
echo "You may be prompted for the password here..."

if sudo chmod a+w /proc/bus/usb/$USBBUS/$USBDEV; then
  echo "Done."
else
  echo "Failed. If you don't have sudo set up, you may need to run this as root."
fi

