#!/bin/bash

# This package is necessary for the miniTLU producer

echo "Entering install_cactus"
echo "Installing cactus:"

export temporary_path=`pwd`

echo $temporary_path

cd $TRAVIS_BUILD_DIR/extern/

if [ $TRAVIS_OS_NAME == linux ]; then 
	
	#sudo apt-get install -y bzip2-devel zlib-devel ncurses-devel openssl openssl-devel python-devel curl curl-devel graphviz graphviz-devel libuuid-devel qt qt-devel PyQt PyQt-devel wxPython gd gd-devel cmake;
	
	if [ -d "$TRAVIS_BUILD_DIR/extern/uhal_2_4_2" ]; then
	
		echo "CACTUS source restored from cache as path exists:"
		
		ls $TRAVIS_BUILD_DIR/extern/uhal_2_4_2
		
	else

		echo "palpidefs source not restored from cache - checking out from CERN svn"
		
		svn co https://svn.cern.ch/reps/cactus/tags/ipbus_sw/uhal_2_4_2
	
	fi

	cd uhal_2_4_2
		
	make Set=uhal

fi


if [ $TRAVIS_OS_NAME == osx ]; then 
	
	
	#sudo apt-get install -y bzip2-devel zlib-devel ncurses-devel openssl openssl-devel python-devel curl curl-devel graphviz graphviz-devel libuuid-devel qt qt-devel PyQt PyQt-devel wxPython gd gd-devel cmake;
	
	if [ -d "$TRAVIS_BUILD_DIR/extern/uhal_2_4_2" ]; then
	
		echo "CACTUS source restored from cache as path exists:"
		
		ls $TRAVIS_BUILD_DIR/extern/uhal_2_4_2
		
	else

		echo "palpidefs source not restored from cache - checking out from CERN svn"
		
		svn co https://svn.cern.ch/reps/cactus/tags/ipbus_sw/uhal_2_4_2
	
	fi

	cd uhal_2_4_2
		
	make Set=uhal

fi

cd $temporary_path

echo "Installed CACTUS"