#!/bin/bash

# Install CERN root

echo "Entering install_root"
echo "Installing CERN root"

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	if [[ $OPTION == 'modern' ]]; then
		export ROOT_FILENAME=${ROOT6_FILENAME_MAC}		
	else
		export ROOT_FILENAME=${ROOT5_FILENAME_MAC}	
	fi

	echo "Installing root now"
	wget https://root.cern.ch/download/$ROOT_FILENAME
	tar -xf $ROOT_FILENAME
	source root/bin/thisroot.sh
	
else
	if [[ $OPTION == 'modern' ]]; then
		export ROOT_FILENAME=${ROOT6_FILENAME_LINUX}
	else
		export ROOT_FILENAME=${ROOT5_FILENAME_LINUX}
	fi
	
	echo "Installing root now"
	wget https://root.cern.ch/download/$ROOT_FILENAME
	tar -xf $ROOT_FILENAME
	source root/bin/thisroot.sh
	
fi
	
echo "Installed CERN root"