#!/bin/bash

# Install cmake
echo "Entered install_cmake script"
echo "Installing cmake"

export temporary_path=`pwd`

cd --

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	
	
	#brew uninstall cmake --force
	#brew unlink cmake
	
	#export CMAKE_FILENAME=${CMAKE_FILENAME_MAC}
	
	#echo "Installing cmake now"
	#wget ${CMAKE_DOWNLOAD_PATH}/$CMAKE_FILENAME
	#tar xfz $CMAKE_FILENAME
	#export PATH="`pwd`/${CMAKE_FILENAME%%.tar.gz}/CMake.app/Contents/bin":$PATH:	
	#echo $PATH
	#mkdir -p /usr/local/Cellar/cmake
	#cp -r ${CMAKE_FILENAME%%.tar.gz}/CMake.app/Contents/* /usr/local/Cellar/cmake/
	
	export PATH="/usr/local/bin:$PATH"
	brew link cmake
	cmake --version
else
	#sudo apt-get install --force-yes -y cmake 
	wget $CMAKE_DOWNLOAD_PATH/$CMAKE_FILENAME_LINUX
	tar -xvf $CMAKE_FILENAME_LINUX
	export PATH="~/${CMAKE_FILENAME_LINUX%.*.*}/bin:$PATH"
fi

cd $temporary_path

echo "Cmake has been installed"