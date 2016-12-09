#!/bin/bash

# Install cmake
echo "Entered install_cmake script"
echo "Installing cmake"

export DEPS_DIR=$HOME/dependencies
export temporary_path=`pwd`

echo $temporary_path

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	
	
	brew uninstall cmake --force
	brew unlink cmake
	
	#export CMAKE_FILENAME=${CMAKE_FILENAME_MAC}
	
	echo "Installing cmake on mac now"
	CMAKE_DIR=${DEPS_DIR}/cmake
	mkdir -p ${CMAKE_DIR}
	cd ${CMAKE_DIR}
	
	wget ${CMAKE_DOWNLOAD_PATH}/$CMAKE_FILENAME_MAC
	tar xfz $CMAKE_FILENAME_MAC --strip-components=3
	export PATH="${CMAKE_DIR}/bin":$PATH	
	#echo $PATH
	#mkdir -p /usr/local/Cellar/cmake
	#cp -r ${CMAKE_FILENAME%%.tar.gz}/CMake.app/Contents/* /usr/local/Cellar/cmake/
	
	#export PATH="/usr/local/bin:$PATH"
	export CMAKE_ROOT="${CMAKE_DIR}"
	#brew link cmake
	#cmake --version
else
	CMAKE_DIR=${DEPS_DIR}/cmake
	mkdir -p ${CMAKE_DIR}
	travis_retry wget --quiet --directory-prefix=${CMAKE_DIR} ${CMAKE_DOWNLOAD_PATH}/${CMAKE_FILENAME_LINUX} 
	ls ${CMAKE_DIR}
	cd ${CMAKE_DIR}
	tar -xf *.tar* --strip-components=1 
	export PATH="${CMAKE_DIR}/bin:${PATH}"
	export CMAKE_ROOT="${CMAKE_DIR}"
fi

cd $temporary_path

echo "Cmake has been installed"
echo `cmake --version` 