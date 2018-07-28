#!/bin/bash

# Install CERN root

echo "Entering install_root"
echo "Installing CERN root"

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	export ROOT_FILENAME=${ROOT6_FILENAME_MAC}

	echo "Installing root now"
	wget https://root.cern.ch/download/$ROOT_FILENAME
	tar -xf $ROOT_FILENAME
	source root/bin/thisroot.sh
	
else
	export ROOT_FILENAME=${ROOT6_FILENAME_LINUX}

	
	echo "Installing root now"
	
	sudo apt-get install --force-yes -y libtbb-dev libcrypto++-dev libcrypto++9v5 libssl-dev libsslcommon2
	 
	wget https://root.cern.ch/download/$ROOT_FILENAME
	tar -xf $ROOT_FILENAME
	source root/bin/thisroot.sh
	
fi
	
echo "Installed CERN root"
