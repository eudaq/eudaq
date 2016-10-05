#!/bin/bash

echo $CXX --version
echo $CC --version

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	# OS X: update brew cache:
	brew update || brew update
	
	if [[ "$CC" == "gcc" ]]; then CC=gcc-4.9; fi	
	
	echo $CXX --version
        echo $CC --version
		
else
	
fi
	
