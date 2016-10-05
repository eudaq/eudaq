#!/bin/bash

# Install openafs / auristor
# last openafs version for OSX 10.9 -> stopped working with 10.11 as there is an error 16 (binary too old) which cannot be handled on the command line

export OPENAFS_DOWNLOAD_PATH_MAC=http://www.openafs.org/dl/openafs/1.6.6/macos-10.9
export OPENAFS_FILENAME_MAC=OpenAFS-1.6.6-Mavericks.dmg
      
if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	brew outdated openssl || brew upgrade openssl
	
	echo "Installing openafs now"
	wget ${OPENAFS_DOWNLOAD_PATH_MAC}/$OPENAFS_FILENAME_MAC
	sudo hdiutil attach $OPENAFS_FILENAME_MAC
	
	ls /Volumes/OpenAFS/
	
	#sudo installer -package /Volumes/OpenAFS/OpenAFS.pkg -target /
	#sudo hdiutil detach /Volumes/OpenAFS
	#sudo launchctl start org.openafs.filesystems.afs
	#tar xfz $OPENAFS_FILENAME_MAC
	#export PATH="`pwd`/${CMAKE_FILENAME%%.tar.gz}/CMake.app/Contents/bin":$PATH:	
	#echo $PATH	
	
else

	#workaround as openafs in the normal is broken in the moment - kernel module does not compile
	sudo add-apt-repository -y ppa:openafs/stable
	sudo apt-get -qq update
	
	sudo apt-get install --force-yes -y linux-generic linux-headers-$(uname -r) openafs-client openafs-krb5	
	
	touch ~/ThisCell
	echo "cern.ch" >> ~/ThisCell
	echo "" >> ~/ThisCell
	sudo cp ~/ThisCell /etc/openafs
	sudo service openafs-client start	
fi
	
