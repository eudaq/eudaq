#!/bin/bash

# TODO: Eithe find a way to reboot the worker on travis or start service without reboot"

# Install openafs / auristor
# last openafs version for OSX 10.9 -> stopped working with 10.11 as there is an error 16 (binary too old) which cannot be handled on the command line

# export OPENAFS_DOWNLOAD_PATH_MAC=http://www.openafs.org/dl/openafs/1.6.6/macos-10.9
# export OPENAFS_FILENAME_MAC=OpenAFS-1.6.6-Mavericks.dmg

echo "Entered install_afs.sh"
echo "Installing afs"

export OPENAFS_DOWNLOAD_PATH_MAC=https://www.auristor.com/downloads/auristor/osx/macos-10.11
export OPENAFS_FILENAME_MAC=AuriStor-client-0.159-ElCapitan.dmg


if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	brew outdated openssl || brew upgrade openssl
	
	echo "Installing openafs now"
	wget ${OPENAFS_DOWNLOAD_PATH_MAC}/$OPENAFS_FILENAME_MAC
	sudo hdiutil attach $OPENAFS_FILENAME_MAC
	
	#ls /Volumes/OpenAFS/
	
	sudo installer -package /Volumes/Auristor-Lite-ElCapitan/Auristor-Lite.pkg -target /
	sudo hdiutil detach /Volumes/Auristor-Lite-ElCapitan
	#sudo launchctl start org.auristor.filesystems.afs
	sudo launchctl list
	sudo launchctl start com.auristor.yfs-client
	sudo launchctl start com.auristor.XPCHelper
	
	ls /afs/
	
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
	
if [[ -d "/afs/desy.de/group/telescopes" ]]; then
	echo "Afs seems to work properly"
elif [[ -d "/afs/cern.ch" ]]; then
	echo "Afs seems to work properly, but desy afs down?"
else
	echo "Something wrong with the afs installation"	
fi
