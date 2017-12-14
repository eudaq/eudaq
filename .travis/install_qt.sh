#!/bin/bash

# Install qt

echo "Entering install_qt"
echo "Installing qt libraries"

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	
	if [[ $OPTION == 'modern' ]]; then
		# Install package dependencies for Mac OS X:
		brew install qt5
		
		export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
		
	else
		# Install package dependencies for Mac OS X:
		#brew update
		brew install qt
		
	fi	
		
else

	if [[ $OPTION == 'modern' ]]; then
		# Install package dependencies for Linux:
		sudo apt-get install --force-yes -y qt5-default
				
	else
		# Install package dependencies for Linux:
		sudo apt-get install --force-yes -y qt4-dev-tools
		
	fi
	
fi
	
echo "Installed qt libraries"