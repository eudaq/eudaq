#!/bin/bash

echo $CXX --version
echo $CC --version

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	# OS X: update brew cache:
	brew update || brew update
	brew outdated openssl || brew upgrade openssl
	
	brew unlink cmake python python3
	
	if [[ "$CC" == "gcc" ]]; then CC=gcc-4.9; fi	
	
	if [[ $OPTION == 'modern' ]]; then
		export ROOT_FILENAME=${ROOT6_FILENAME_MAC}
		
#		brew upgrade pyenv
#		brew install homebrew/boneyard/pyenv-pip-rehash
#		brew install pyenv-virtualenv
#		brew install pyenv-virtualenvwrapper
		
#		pyenv init -
#		pyenv virtualenv-init -
		
		#cat "eval \"$(pyenv init -)\"" >> ~/.bashrc
		#cat "eval \"$(pyenv virtualenv-init -)\"" >> ~/.bashrc
		#cat ". ~/.bashrc" >> ~./bash_profile
		
		#source ~/.bashrc
		#exec $SHELL
		
#		echo "Installing pyenv"
#		pyenv install 3.5.0
#		pyenv global 3.5.0
#		pyenv versions
		
#		echo "Installing virtualenv plugin"
#		pyenv virtualenv 3.5.0 my-virtual-env
#		pyenv virtualenvs
#		pyenv activate my-virtual-env
		#cd my-virtual-env
		#source bin/activate

		# install pyenv
		# https://github.com/pyca/cryptography/blob/master/.travis/install.sh
		git clone https://github.com/yyuu/pyenv.git ~/.pyenv
		PYENV_ROOT="$HOME/.pyenv"
		PATH="$PYENV_ROOT/bin:$PATH"
		eval "$(pyenv init -)"
		
		pyenv install 3.5.1
		pyenv global 3.5.1
		
		pyenv rehash
		python -m pip install --user virtualenv		

		python -m virtualenv ~/.venv
		source ~/.venv/bin/activate
		
		pip install --upgrade pip
		pip install -q numpy
		
	else
		export ROOT_FILENAME=${ROOT5_FILENAME_MAC}
		# install pyenv
		# https://github.com/pyca/cryptography/blob/master/.travis/install.sh
		git clone https://github.com/yyuu/pyenv.git ~/.pyenv
		PYENV_ROOT="$HOME/.pyenv"
		PATH="$PYENV_ROOT/bin:$PATH"
		eval "$(pyenv init -)"
		
		pyenv install 2.7.10
		pyenv global 2.7.10
		
		pyenv rehash
		python -m pip install --user virtualenv		

		python -m virtualenv ~/.venv
		source ~/.venv/bin/activate
		
		pip install --upgrade pip
		pip install -q numpy		
	fi
	export CMAKE_FILENAME=${CMAKE_FILENAME_MAC}

	echo "Installing root now"
	wget https://root.cern.ch/download/$ROOT_FILENAME
	tar -xvf $ROOT_FILENAME
	source root/bin/thisroot.sh
	
	echo "Installing cmake now"
	wget ${CMAKE_DOWNLOAD_PATH}/$CMAKE_FILENAME
	tar xfz $CMAKE_FILENAME
	export PATH="`pwd`/${CMAKE_FILENAME%%.tar.gz}/CMake.app/Contents/bin":$PATH:	
	echo $PATH
	
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
	if [[ $OPTION == 'modern' ]]; then
		export ROOT_FILENAME=${ROOT6_FILENAME_LINUX}
		
		export PIP_REQUIRE_VIRTUALENV=true
		
		pyenv install 3.5.0
		pyenv global 3.5.0
		pyenv versions
		
		git clone https://github.com/yyuu/pyenv-pip-rehash.git ~/.pyenv/plugins/pyenv-pip-rehash
		git clone https://github.com/yyuu/pyenv-virtualenv.git ~/.pyenv/plugins/pyenv-virtualenv
		#eval "$(pyenv init -)"
		#eval "$(pyenv virtualenv-init -)"
		pyenv init
		pyenv virtualenv-init
		#pyenv virtualenv-init -
		
		pyenv virtualenv 3.5.0 my-virtual-env
		pyenv virtualenvs
		pyenv activate my-virtual-env
		
		
		pip install --upgrade pip
		#pip install virtualenvwrapper
		#pyvenv  venv
		#pyenv virtualenvs
		#source venv/bin/activate
		#pyenv activate venv
		pip install -q numpy
		#pyenv rehash
	else
		export ROOT_FILENAME=${ROOT5_FILENAME_LINUX}
		
		pyenv install 2.7.10
		pyenv global 2.7.10
		pyenv versions
		
		git clone https://github.com/yyuu/pyenv-pip-rehash.git ~/.pyenv/plugins/pyenv-pip-rehash
		git clone https://github.com/yyuu/pyenv-virtualenv.git ~/.pyenv/plugins/pyenv-virtualenv
		pyenv init
		pyenv virtualenv-init
		#pyenv virtualenv-init -
		
		pyenv virtualenv 2.7.10 my-virtual-env
		pyenv virtualenvs
		pyenv activate my-virtual-env
		
		
		pip install --upgrade pip
		pip install virtualenvwrapper
		#pyvenv  venv
		#pyenv virtualenvs
		#source venv/bin/activate
		#pyenv activate venv
		pip install -q numpy
		#pyenv rehash		
	fi
	
	echo "Installing root now"
	wget https://root.cern.ch/download/$ROOT_FILENAME
	tar -xvf $ROOT_FILENAME
	source root/bin/thisroot.sh
	
	#workaround as openafs in the normal is broken in the moment - kernel module does not compile
	sudo add-apt-repository -y ppa:openafs/stable
	sudo apt-get -qq update
fi
	
