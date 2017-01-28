#!/bin/bash

# This package is necessary for the palpidefs producer

echo "Entering install_palpidefs_driver"
echo "Installing palpidefs driver:"

export temporary_path=`pwd`

echo $temporary_path

cd $TRAVIS_BUILD_DIR/extern/

if [ $TRAVIS_OS_NAME == linux ]; then 
	
	sudo apt-get update && sudo apt-get install -y libtinyxml-dev expect-dev libusb-1.0-0-dev; 

	if [ -d "$TRAVIS_BUILD_DIR/extern/aliceitsalpidesoftware/pALPIDEfs-software" ]; then
	
		echo "palpidefs source restored from cache as path exists:"
		
		zip -r aliceitsalpidesoftware.zip aliceitsalpidesoftware
		
	else
		echo "palpidefs source not restored from cache - downloading from CERNBOX and unpacking"
		
		wget -O alice-its-alpide-software.zip https://cernbox.cern.ch/index.php/s/QIRPTV84XziyQ3q/download
		
		unzip alice-its-alpide-software.zip
	
		rm -rf aliceitsalpidesoftware

		mv alice-its-alpide-software-* aliceitsalpidesoftware
	
		zip -r aliceitsalpidesoftware.zip aliceitsalpidesoftware
	fi

	echo `ls `
	
	cd aliceitsalpidesoftware
	cd pALPIDEfs-software

	sed -i '2s/.*//' Makefile
	
	make lib

fi

# http://apple.stackexchange.com/questions/193138/to-install-unbuffer-in-osx

if [ $TRAVIS_OS_NAME == osx ]; then 

	brew update
	#brew --prefix
	#brew -ls verbose tinyxml
	#brew install tinyxml homebrew/dupes/expect libusb
	brew install homebrew/dupes/expect libusb
	
	#if [ -d "$TRAVIS_BUILD_DIR/extern/tinyxml" ]; then
	
	#	echo "tinyxml source restored from cache as path exists:"
		
	#else
		echo "tinyxml source not restored from cache - downloading from sourceforge and unpacking"
		
		wget -O tinyxml_2_6_2.zip https://downloads.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip
		
		unzip tinyxml_2_6_2.zip

	#fi

	cd tinyxml

	sed -i.bak '22s/.*//' Makefile
	sed -i.bak '23s/.*//' Makefile
	sed -i.bak '24s/.*//' Makefile
	sed -i.bak '25s/.*//' Makefile
	sed -i.bak '26s/.*//' Makefile	
	sed -i.bak '37s/.*/RELEASE_LDFLAGS  := -macosx_version_min 10.11/' Makefile	
	
	make
	
	export PATH=/Users/travis/build/eudaq/eudaq/extern/tinyxml:$PATH
	export CFLAGS="-I /Users/travis/build/eudaq/eudaq/extern/tinyxml" $CFLAGS
	export LDFLAGS="-L /Users/travis/build/eudaq/eudaq/extern/tinyxml" $LDFLAGS
	
	pwd
	
	ls
	
	
	
	mv tinystr.o tinystr.a
	mv tinyxmlerror.o tinyxmlerror.a
	mv tinyxmlparser.o tinyxmlparser.a
	mv tinyxml.o tinyxml.a
	
	cd ..	

	if [ -d "$TRAVIS_BUILD_DIR/extern/aliceitsalpidesoftware/pALPIDEfs-software" ]; then
		
		echo "palpidefs source restored from cache as path exists:"
		
		zip -r aliceitsalpidesoftware.zip aliceitsalpidesoftware
		
	else
		echo "palpidefs source not restored from cache - downloading from CERNBOX and unpacking"
		
		wget -O alice-its-alpide-software.zip https://cernbox.cern.ch/index.php/s/QIRPTV84XziyQ3q/download
		
		unzip alice-its-alpide-software.zip
	
		rm -rf aliceitsalpidesoftware

		mv alice-its-alpide-software-* aliceitsalpidesoftware
	
		zip -r aliceitsalpidesoftware.zip aliceitsalpidesoftware
	fi

	cd $TRAVIS_BUILD_DIR/extern/aliceitsalpidesoftware
	cd pALPIDEfs-software

	sed -i '' '2s/.*/GIT_VERSION:=\"e1b12f7\"/' Makefile
	sed -i '' '3s/.*/CFLAGS= -I\/Users\/travis\/build\/eudaq\/eudaq\/extern\/tinyxml -pipe -fPIC -DVERSION=\\"$(GIT_VERSION)\\" -g -std=c++11/' Makefile
	sed -i '' '4s/.*/LINKFLAGS=\/Users\/travis\/build\/eudaq\/eudaq\/extern\/tinyxml\/tinystr.a \/Users\/travis\/build\/eudaq\/eudaq\/extern\/tinyxml\/tinyxmlerror.a \/Users\/travis\/build\/eudaq\/eudaq\/extern\/tinyxml\/tinyxmlparser.a \/Users\/travis\/build\/eudaq\/eudaq\/extern\/tinyxml\/tinyxml.a -lusb-1.0 /' Makefile		
	
	sed -i '' '308s/.*//' c_wrapper.cpp		
	
	make lib
	
fi

pwd

cd $temporary_path

echo "Installed palpidefs driver"
