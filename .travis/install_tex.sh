#!/bin/bash

echo "Entering install_tex"
echo "Installing tex system"

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	if [[ $BUILD_manual == 'ON' ]]; then
		echo "Installing texlive under OSX not prepared yet..."
	fi	
	
else 
	
	if [[ $BUILD_manual == 'ON' ]]; then
		echo "Installing texlive as manual will be build..."
		sudo apt-get install -y texlive texlive-latex-extra
	fi
	
fi 

echo "Installed tex system"
