#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
  # Install package dependencies for Mac OS X:
   brew unlink cmake
   brew install python libusb qt
  # Install numpy via pip:
   easy_install pip
   pip install numpy
else 
  # Install package dependencies for Linux:
  sudo apt-get install -y libusb-1.0-0 libusb-1.0-0-dev python2.7 python-numpy cmake qt4-dev-tools
fi 


