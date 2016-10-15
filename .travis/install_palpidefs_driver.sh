#!/bin/bash

# This package is necessary for the palpidefs producer

echo "Entering install_palpidefs_driver"
echo "Installing palpidefs driver"

export temporary_path=`pwd`

if [ $TRAVIS_OS_NAME == linux ]; then sudo apt-get update && sudo apt-get install -y libtinyxml-dev expect-dev libusb-1.0-0-dev; fi

# http://apple.stackexchange.com/questions/193138/to-install-unbuffer-in-osx

if [ $TRAVIS_OS_NAME == osx ]; then brew update && brew --prefix && brew -ls verbose tinyxml && brew install tinyxml homebrew/dupes/expect libusb; fi

cd --

wget -O alice-its-alpide-software-master-latest.zip https://cernbox.cern.ch/index.php/s/QIRPTV84XziyQ3q/download

unzip alice-its-alpide-software-master-latest.zip

cd alice-its-alpide-software-master-3189f00d7515733d46a61a5ab2606e436df4955b

cd pALPIDEfs-software

make lib

cd $temporary_path

echo "Installed palpidefs driver"