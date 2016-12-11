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
		brew install cartr/qt4/qt
		#FIXME qt4 no longer officially supported upstream, no longer supported in homebrew on Sierra upwards --> retire it soon
		#http://stackoverflow.com/questions/39690404/brew-install-qt-does-not-work-on-macos-sierra
		#https://github.com/Homebrew/homebrew-core/pull/5216
		# brew install qt
		
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