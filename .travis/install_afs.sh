#!/bin/bash

# Install openafs / auristor

export OPENAFS_DOWNLOAD_PATH_MAC=http://www.openafs.org/dl/openafs/1.6.6/macos-10.9
export OPENAFS_FILENAME_MAC=OpenAFS-1.6.6-Mavericks.dmg
      
if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	# OS X: update brew cache:
	brew update || brew update
	
	echo "Installing openafs now"
	wget ${OPENAFS_DOWNLOAD_PATH_MAC}/$OPENAFS_FILENAME_MAC
	sudo hdiutil attach $OPENAFS_FILENAME_MAC
	#ls /Volumes/OpenAFS/
	sudo installer -package /Volumes/OpenAFS/OpenAFS.pkg -target /
	sudo hdiutil detach /Volumes/OpenAFS
	sudo launchctl start org.openafs.filesystems.afs
	#tar xfz $OPENAFS_FILENAME_MAC
	#export PATH="`pwd`/${CMAKE_FILENAME%%.tar.gz}/CMake.app/Contents/bin":$PATH:	
	#echo $PATH	
	
else

	#workaround as openafs in the normal is broken in the moment - kernel module does not compile
	sudo add-apt-repository -y ppa:openafs/stable
	sudo apt-get -qq update
fi
	
