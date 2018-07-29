#!/bin/bash

# Install libusb 1.0

echo "Entering install_libusb_1_0"
echo "Installing libusb 1.0"

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	# Install package dependencies for Mac OS X:
	#brew update
	brew install libusb

else

	# Install package dependencies for Linux:
	sudo apt-get install --force-yes -y libusb-1.0-0 libusb-1.0-0-dev

fi

echo "Installed libusb 1.0"
