#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	if [[ $OPTION == 'modern' ]]; then

	else

	fi
	
else 
	
	if [[ $BUILD_manual == 'ON' ]]; then
		echo "Installing texlive as manual will be build..."
		sudo apt-get install -y texlive texlive-latex-extra
	fi
	
fi 


