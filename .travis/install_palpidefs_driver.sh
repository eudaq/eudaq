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
		
		wget -O alice-its-alpide-software.zip https://cernbox.cern.ch/index.php/s/MMI1rIRO7EV1ued/download

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

	wget -O tinyxml_2_6_2.zip http://downloads.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip

	unzip tinyxml_2_6_2.zip

	cd tinyxml

	make
	
	export PATH=/Users/travis/tinyxml:$PATH
	export CFLAGS="-I /Users/travis/tinyxml" $CFLAGS
	export LDFLAGS="-L /Users/travis/tinyxml" $LDFLAGS
	
	mv /Users/travis/tinyxml/tinystr.o /Users/travis/tinyxml/tinystr.a
	mv /Users/travis/tinyxml/tinyxmlerror.o /Users/travis/tinyxml/tinyxmlerror.a
	mv /Users/travis/tinyxml/tinyxmlparser.o /Users/travis/tinyxml/tinyxmlparser.a
	mv /Users/travis/tinyxml/tinyxml.o /Users/travis/tinyxml/tinyxml.a
	
	cd ..	

	if [ ! -d $TRAVIS_BUILD_DIR/extern/aliceitsalpidesoftware ]; then
		wget -O alice-its-alpide-software.zip https://cernbox.cern.ch/index.php/s/MMI1rIRO7EV1ued/download

		unzip alice-its-alpide-software.zip
		mv alice-its-alpide-software-* aliceitsalpidesoftware
		zip -r aliceitsalpidesoftware.zip aliceitsalpidesoftware
	fi

	cd aliceitsalpidesoftware
	cd pALPIDEfs-software

	sed -i '' '2s/.*/GIT_VERSION:=\"e1b12f7\"/' Makefile
	sed -i '' '3s/.*/CFLAGS= -I\/Users\/travis\/tinyxml -pipe -fPIC -DVERSION=\\"$(GIT_VERSION)\\" -g -std=c++11/' Makefile
	sed -i '' '4s/.*/LINKFLAGS=\/Users\/travis\/tinyxml\/tinystr.a \/Users\/travis\/tinyxml\/tinyxmlerror.a \/Users\/travis\/tinyxml\/tinyxmlparser.a \/Users\/travis\/tinyxml\/tinyxml.a -lusb-1.0 /' Makefile		
	
	sed -i '' '308s/.*//' c_wrapper.cpp		
	
	make lib
	
fi

pwd

cd $temporary_path

echo "Installed palpidefs driver"
