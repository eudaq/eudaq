#!/bin/bash

echo "Entered update repositories script"
echo "Package managers will be updated"
echo ""

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	# OS X: update brew cache:
	echo "General brew upgrade"
	brew update || brew upgrade
	
	echo "Upgrading pip itself"
	python -m pip install -U pip
	
else
	echo "Apt upgrade"
	export DEBIAN_FRONTEND=noninteractive
	sudo apt-get update
	sudo apt-get  --force-yes -y upgrade 
	
fi
	
