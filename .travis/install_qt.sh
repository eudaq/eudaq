#!/bin/bash

# Install qt

echo "Entering install_qt"
echo "Installing qt libraries"

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	# Install package dependencies for Mac OS X:
	brew install qt5
	export CMAKE_PREFIX_PATH=/usr/local/opt/qt5

else

	# Install package dependencies for Linux:
	sudo apt-get install --force-yes -y qt5-default

fi

echo "Installed qt libraries"
