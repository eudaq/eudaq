#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	if [[ $OPTION == 'modern' ]]; then
		# Install package dependencies for Mac OS X:
		brew unlink cmake
		brew install python3 libusb qt5 libusb-compat
		export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
		# Install numpy via pip:
		easy_install pip
		pip install numpy
	else
		# Install package dependencies for Mac OS X:
		brew unlink cmake
		brew install python libusb qt libusb-compat
		# Install numpy via pip:
		easy_install pip
		pip install numpy	
	fi
	
else 

	if [[ $OPTION == 'modern' ]]; then
		# Install package dependencies for Linux:
		sudo apt-get install -y libusb-dev libusb-1.0-0 libusb-1.0-0-dev python3.4 python-numpy cmake qt5-default linux-generic linux-headers-$(uname -r) openafs-client openafs-krb5
		sudo service openafs-client start
	else
		# Install package dependencies for Linux:
		sudo apt-get install -y libusb-dev libusb-1.0-0 libusb-1.0-0-dev python2.7 python-numpy cmake qt4-dev-tools linux-generic linux-headers-$(uname -r) openafs-client openafs-krb5
		sudo service openafs-client start
	fi
	
	if [[ $BUILD_manual == 'ON' ]]; then
		echo "Installing texlive as manual will be build..."
		sudo apt-get install -y texlive texlive-latex-extra
	fi
	
fi 


