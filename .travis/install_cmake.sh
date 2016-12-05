#!/bin/bash

# Install cmake
echo "Entered install_cmake script"
echo "Installing cmake"

export DEPS_DIR=$HOME/dependencies

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
	CMAKE_DIR=${DEPS_DIR}/cmake
	mkdir -p ${CMAKE_DIR}
	travis_retry wget --quiet --directory-prefix=${CMAKE_DIR} ${CMAKE_DOWNLOAD_PATH}/${CMAKE_FILENAME_LINUX} 
	ls ${CMAKE_DIR}
	tar -xf -C ${CMAKE_DIR} ${CMAKE_DIR}/*.tar* --strip-components=1 
	export PATH="${CMAKE_DIR}/bin:${PATH}"
	export CMAKE_ROOT="${CMAKE_DIR}"
fi

echo "Cmake has been installed"
echo `cmake --version` 