#!/bin/bash

echo "Entering install_tex"
echo "Installing tex system"

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	if [[ $EUDAQ_BUILD_MANUAL == 'ON' ]]; then
		echo "Installing texlive under OSX not prepared yet..."
	fi	
	
else 
	
	if [[ $EUDAQ_BUILD_MANUAL == 'ON' ]]; then
		echo "Installing texlive as manual will be build..."
		sudo apt-get install -y texlive texlive-plain-generic texlive-latex-extra poppler-utils latex2html doxygen
	fi
	
fi 

echo "Installed tex system"
