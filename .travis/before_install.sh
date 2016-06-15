#!/bin/bash

echo $CXX --version
echo $CC --version

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	if [[ $OPTION == 'modern' ]]; then
		export ROOT_FILENAME=${ROOT6_FILENAME_MAC}
	else
		export ROOT_FILENAME=${ROOT5_FILENAME_MAC}
	fi
	export CMAKE_FILENAME=${CMAKE_FILENAME_MAC}

	echo "Installing root now"
	wget https://root.cern.ch/download/$ROOT_FILENAME
	tar -xvf $ROOT_FILENAME
	source root/bin/thisroot.sh
	
	echo "Installing cmake now"
	wget ${CMAKE_DOWNLOAD_PATH}/$CMAKE_FILENAME
	tar xfz $CMAKE_FILENAME
	export PATH="`pwd`/${CMAKE_FILENAME%%.tar.gz}/CMake.app/Contents/bin":$PATH:	
	echo $PATH
	
	# OS X: update brew cache:
	brew update
	
	if [[ "$CC" == "gcc" ]]; then CC=gcc-4.9; fi
	
	echo "Installing openafs now"
	wget ${OPENAFS_DOWNLOAD_PATH_MAC}/$OPENAFS_FILENAME_MAC
	sudo hdiutil attach $OPENAFS_FILENAME_MAC
	ls /Volumes/OpenAFS/
	sudo installer -package /Volumes/OpenAFS/OpenAFS.pkg -target /
	#tar xfz $OPENAFS_FILENAME_MAC
	#export PATH="`pwd`/${CMAKE_FILENAME%%.tar.gz}/CMake.app/Contents/bin":$PATH:	
	#echo $PATH	
	
else
	if [[ $OPTION == 'modern' ]]; then
		export ROOT_FILENAME=${ROOT6_FILENAME_LINUX}
	else
		export ROOT_FILENAME=${ROOT5_FILENAME_LINUX}
	fi
	
	echo "Installing root now"
	wget https://root.cern.ch/download/$ROOT_FILENAME
	tar -xvf $ROOT_FILENAME
	source root/bin/thisroot.sh
	
	#workaround as openafs in the normal is broken in the moment - kernel module does not compile
	sudo add-apt-repository -y ppa:openafs/stable
	sudo apt-get -qq update
fi
	
