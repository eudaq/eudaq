#!/bin/bash

echo "Entered update repositories script"
echo "Package managers will be updated"
echo ""

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	# OS X: update brew cache:
	echo "General brew upgrade"
	brew update || brew upgrade
	
	#echo "Upgrading pip itself"
	#python -m pip install -U pip
	
else
	echo "Apt upgrade"
	export DEBIAN_FRONTEND=noninteractive
	#sudo apt-get update
	#http://askubuntu.com/questions/104899/make-apt-get-or-aptitude-run-with-y-but-not-prompt-for-replacement-of-configu
	echo apt-get update and upgrade disabled temporarily due to error - 25.08.2017
	#sudo apt-get  -o Dpkg::Options::="--force-confnew" --force-yes -y upgrade 
	
fi
	
