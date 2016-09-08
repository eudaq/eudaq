#!/bin/bash

# This package is necessary for the CMS pixel option

export temporary_path=`pwd`

if [ $TRAVIS_OS_NAME == linux ]; then sudo apt-get update && sudo apt-get install -y libusb-1.0-0 libusb-1.0-0-dev libftdi1 libftdi-dev; fi

if [ $TRAVIS_OS_NAME == osx ]; then brew update && brew install libusb libftdi; fi

cd --

git clone https://github.com/simonspa/pxar.git

cd pxar

git checkout testbeam-2016

mkdir build

cd build

cmake ..

make install

export PXARPATH=~/pxar

cd $temporary_path