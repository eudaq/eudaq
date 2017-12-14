#!/bin/bash

# This package is necessary for the rpi-controller option

echo "Entered install_wiringpi script"
echo "Installing wiringPi library"

export temporary_path=`pwd`

#if [ $TRAVIS_OS_NAME == linux ]; then sudo apt-get update && sudo apt-get install -y libusb-1.0-0 libusb-1.0-0-dev libftdi1 libftdi-dev; fi

#if [ $TRAVIS_OS_NAME == osx ]; then brew update && brew install libusb libftdi; fi

cd --

git clone git://git.drogon.net/wiringPi

cd wiringPi

sudo ./build

cd $temporary_path

echo "Installed wiringPi library"