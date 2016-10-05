#!/bin/bash

# Install libusb 0.1

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	if [[ $OPTION == 'modern' ]]; then
		# Install package dependencies for Mac OS X:
		#brew update
		brew install libusb-compat
		
	else
		# Install package dependencies for Mac OS X:
		#brew update
		brew install libusb-compat
		
	fi	
		
else

	if [[ $OPTION == 'modern' ]]; then
		# Install package dependencies for Linux:
		sudo apt-get install --force-yes -y libusb-dev
				
	else
		# Install package dependencies for Linux:
		sudo apt-get install --force-yes -y libusb-dev
		
	fi
	
fi
	
