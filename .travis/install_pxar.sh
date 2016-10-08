#!/bin/bash

# This package is necessary for the CMS pixel option

echo "Entering install_pxar"
echo "Installing pxar library"

export temporary_path=`pwd`

if [ $TRAVIS_OS_NAME == linux ]; then sudo apt-get install -y libusb-1.0-0 libusb-1.0-0-dev libftdi1 libftdi-dev; fi

if [ $TRAVIS_OS_NAME == osx ]; then brew install libusb libftdi; fi

cd --

git clone https://github.com/simonspa/pxar.git

cd pxar

git checkout testbeam-2016

mkdir build

cd build

cmake -DBUILD_pxarui=OFF ..

make install

export PXARPATH=~/pxar

cd $temporary_path

echo "Installed pxar library"