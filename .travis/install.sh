#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	if [[ $OPTION == 'modern' ]]; then
		# Install package dependencies for Mac OS X:
		brew update
		brew unlink cmake python python3
		brew install libusb qt5 libusb-compat
		
		brew install python3
		brew linkapps python3
		#export python_workaround = cmake -DPYTHON_LIBRARY=$(python-config --prefix)/lib/libpython2.7.dylib -DPYTHON_INCLUDE_DIR=$(python-config --prefix)/include/python2.7
		export OVERRIDE_PYTHON_INTERPRETER=-DPYTHON_EXECUTABLE:FILEPATH=$(python-config --prefix)"/bin/python3"
		export OVERRIDE_PYTHON_LIBRARY_PATH=-DPYTHON_LIBRARY=$(python-config --prefix)"/lib/libpython3.4.dylib"
		
		export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
		# Install numpy via pip:
		sudo pip install --upgrade pip
		sudo pip install -q numpy
	else
		# Install package dependencies for Mac OS X:
		brew update
		brew unlink cmake python python3
		brew install libusb qt libusb-compat
		
		brew install python
		brew linkapps python
		export OVERRIDE_PYTHON_INTERPRETER=-DPYTHON_EXECUTABLE:FILEPATH=$(python-config --prefix)"/bin/python"
		export OVERRIDE_PYTHON_LIBRARY_PATH=-DPYTHON_LIBRARY=$(python-config --prefix)"/lib/libpython2.7.dylib"

		# Install numpy via pip:
		sudo pip install --upgrade pip
		sudo pip install -q numpy	
	fi
	
else 

	if [[ $OPTION == 'modern' ]]; then
		# Install package dependencies for Linux:
		sudo apt-get install -y python3 python3-pip libusb-dev libusb-1.0-0 libusb-1.0-0-dev cmake qt5-default linux-generic linux-headers-$(uname -r) openafs-client openafs-krb5
		sudo service openafs-client start
		sudo pip3 install -U setuptools
		sudo pip3 install -U virtualenvwrapper
		python3 -V
		pip3 -V
		sudo pip3 install -q numpy
		#export OVERRIDE_PYTHON_INTERPRETER="-DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/python3"
		#export OVERRIDE_PYTHON_LIBRARY_PATH="-DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.4m.so.1"
		PATH=$(echo $PATH | tr ':' "\n" | sed '/\/opt\/python/d' | tr "\n" ":" | sed "s|::|:|g")
		PATH=/opt/python/3.5.0/bin:$PATH
		
	else
		# Install package dependencies for Linux:
		sudo apt-get install -y python2 libusb-dev libusb-1.0-0 libusb-1.0-0-dev cmake qt4-dev-tools linux-generic linux-headers-$(uname -r) openafs-client openafs-krb5
		sudo service openafs-client start
		sudo pip install --upgrade pip
		sudo pip install -q numpy
		#export OVERRIDE_PYTHON_INTERPRETER="-DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/python"
		#export OVERRIDE_PYTHON_LIBRARY_PATH="-DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython2.7.so"
		PATH=$(echo $PATH | tr ':' "\n" | sed '/\/opt\/python/d' | tr "\n" ":" | sed "s|::|:|g")
		PATH=/opt/python/2.7.10/bin:$PATH
	fi
	
	if [[ $BUILD_manual == 'ON' ]]; then
		echo "Installing texlive as manual will be build..."
		sudo apt-get install -y texlive texlive-latex-extra
	fi
	
fi 


