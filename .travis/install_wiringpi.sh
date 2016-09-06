#!/bin/bash

# This package is necessary for the rpi-controller option

export temporary_path=`pwd`

echo "Installing wiringPi library"

#if [ $TRAVIS_OS_NAME == linux ]; then sudo apt-get update && sudo apt-get install -y libusb-1.0-0 libusb-1.0-0-dev libftdi1 libftdi-dev; fi

#if [ $TRAVIS_OS_NAME == osx ]; then brew update && brew install libusb libftdi; fi

cd --

git clone git://git.drogon.net/wiringPi

cd wiringPi

sudo ./build

cd $temporary_path