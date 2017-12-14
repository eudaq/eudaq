#!/bin/bash

echo "Entered set_compilers script"
echo "Compiler versions will be installed and set"
echo ""

echo "Current C++/C compilers"

echo `$CXX --version`
echo `$CC --version`

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	echo "Setting C-Compiler to 4.9 for OSX"
	if [[ "$CC" == "gcc" ]]; then CC=gcc-4.9; fi	
	
fi
	
echo "Current C++/C compilers after settings in set_compilers"

echo `$CXX --version`
echo `$CC --version`