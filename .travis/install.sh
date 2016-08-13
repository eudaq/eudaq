#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	if [[ $OPTION == 'modern' ]]; then
		# Install package dependencies for Mac OS X:
		#brew update
		brew unlink cmake python python3
		brew install libusb qt5 libusb-compat
		
		#brew install python3
		#brew linkapps python3
		#export python_workaround = cmake -DPYTHON_LIBRARY=$(python-config --prefix)/lib/libpython2.7.dylib -DPYTHON_INCLUDE_DIR=$(python-config --prefix)/include/python2.7
		#export OVERRIDE_PYTHON_INTERPRETER=-DPYTHON_EXECUTABLE:FILEPATH=$(python-config --prefix)"/bin/python3"
		#export OVERRIDE_PYTHON_LIBRARY_PATH=-DPYTHON_LIBRARY=$(python-config --prefix)"/lib/libpython3.4.dylib"
		
		export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
		# Install numpy via pip:
		#sudo pip install --upgrade pip
		#sudo pip install -q numpy
	else
		# Install package dependencies for Mac OS X:
		#brew update
		brew unlink cmake python python3
		brew install libusb qt libusb-compat
		
		#brew install python
		#brew linkapps python
		#export OVERRIDE_PYTHON_INTERPRETER=-DPYTHON_EXECUTABLE:FILEPATH=$(python-config --prefix)"/bin/python"
		#export OVERRIDE_PYTHON_LIBRARY_PATH=-DPYTHON_LIBRARY=$(python-config --prefix)"/lib/libpython2.7.dylib"

		# Install numpy via pip:
		#sudo pip install --upgrade pip
		#sudo pip install -q numpy	
	fi
	
else 

	if [[ $OPTION == 'modern' ]]; then
		# Install package dependencies for Linux:
		sudo add-apt-repository -y ppa:openafs/stable
		sudo apt-get update
		sudo apt-get install --force-yes -y libusb-dev libusb-1.0-0 libusb-1.0-0-dev cmake qt5-default linux-generic linux-headers-$(uname -r) openafs-client openafs-krb5
		
		# creating ThisCell file --> since some update it seems to be absolutely necessary, otherwise start up fails with
		# "afsd: some file missing or bad in /etc/openafs"
		# content does not matter for our purposes		
		touch ~/ThisCell
		echo "cern.ch" >> ~/ThisCell
		echo "" >> ~/ThisCell
		sudo cp ~/ThisCell /etc/openafs
		sudo service openafs-client start
		
		#sudo cat /var/log/syslog
		#sudo pip install -U setuptools
		#sudo pip install -U virtualenvwrapper
		#sudo virtualenv /opt/python/3.5.0
		#source /opt/python/3.5.0/activate
		#sudo pip install -q numpy
		#export OVERRIDE_PYTHON_INTERPRETER="-DPYTHON_EXECUTABLE:FILEPATH=/opt/python/3.5.0/bin/python"
		#export OVERRIDE_PYTHON_LIBRARY_PATH="-DPYTHON_LIBRARY=/opt/python/3.5.0/lib/libpython3.4m.so.1"
		#export OVERRIDE_PYTHON_INCLUDE_DIR="-DPYTHON_INCLUDE_DIR=/opt/python/3.5.0/include"
		#PATH=$(echo $PATH | tr ':' "\n" | sed '/\/opt\/python/d' | tr "\n" ":" | sed "s|::|:|g")
		#PATH=/opt/python/3.5.0/bin:$PATH
		
	else
		# Install package dependencies for Linux:
		sudo add-apt-repository -y ppa:openafs/stable
		sudo apt-get update		
		sudo apt-get install --force-yes -y libusb-dev libusb-1.0-0 libusb-1.0-0-dev cmake qt4-dev-tools linux-generic linux-headers-$(uname -r) openafs-client openafs-krb5
		
		# creating ThisCell file --> since some update it seems to be absolutely necessary, otherwise start up fails with
		# "afsd: some file missing or bad in /etc/openafs"
		# content does not matter for our purposes
		touch ~/ThisCell
		echo "cern.ch" >> ~/ThisCell
		echo "" >> ~/ThisCell
		sudo cp ~/ThisCell /etc/openafs		
		sudo service openafs-client start
		
		#sudo cat /var/log/syslog
		#sudo pip install --upgrade pip
		#sudo pip install -q numpy
		#pyenv rehash
		#export OVERRIDE_PYTHON_INTERPRETER="-DPYTHON_EXECUTABLE:FILEPATH=/opt/python/2.7.10/bin/python"
		#export OVERRIDE_PYTHON_LIBRARY_PATH="-DPYTHON_LIBRARY=/opt/python/2.7.10/lib/libpython2.7.so"
		#export OVERRIDE_PYTHON_INCLUDE_DIR="-DPYTHON_LIBRARY=/opt/python/2.7.10/include"
		#PATH=$(echo $PATH | tr ':' "\n" | sed '/\/opt\/python/d' | tr "\n" ":" | sed "s|::|:|g")
		#PATH=/opt/python/2.7.10/bin:$PATH
	fi
	
	if [[ $BUILD_manual == 'ON' ]]; then
		echo "Installing texlive as manual will be build..."
		sudo apt-get install -y texlive texlive-latex-extra
	fi
	
fi 


