#!/bin/bash

echo $CXX --version
echo $CC --version

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	wget https://root.cern.ch/download/root_v5.34.36.macosx64-10.9-clang60.tar.gz 
	tar -xvf root_v5.34.36.macosx64-10.9-clang60.tar.gz
	source root/bin/thisroot.sh
	
	# OS X: update brew cache:
	brew update
	
	if [[ "$CC" == "gcc" ]]; then CC=gcc-4.9; fi
	
	wget https://cmake.org/files/v3.4/cmake-3.4.3-Darwin-x86_64.tar.gz
	tar xfz cmake-3.4.3-Darwin-x86_64.tar.gz
	export PATH="`pwd`/cmake-3.4.3-Darwin-x86_64/CMake.app/Contents/bin":$PATH:	
	echo $PATH
else
	wget https://root.cern.ch/download/root_v5.34.36.Linux-ubuntu14-x86_64-gcc4.8.tar.gz
	tar -xvf root_v5.34.36.Linux-ubuntu14-x86_64-gcc4.8.tar.gz
	source root/bin/thisroot.sh
	
	sudo apt-get -qq update
fi
