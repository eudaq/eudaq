#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	if [[ $BUILD_manual == 'ON' ]]; then
		echo "Installing texlive under OSX not prepared yet..."
		sudo apt-get install -y texlive texlive-latex-extra
	fi	
	
else 
	
	if [[ $BUILD_manual == 'ON' ]]; then
		echo "Installing texlive as manual will be build..."
		sudo apt-get install -y texlive texlive-latex-extra
	fi
	
fi 


