#!/bin/bash

# Install python

echo "Entering install_python script"
echo "Installing python"

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	
	brew unlink python python3
	
	if [[ $OPTION == 'modern' ]]; then
		# install pyenv
		# https://github.com/pyca/cryptography/blob/master/.travis/install.sh
		git clone https://github.com/yyuu/pyenv.git ~/.pyenv
		PYENV_ROOT="$HOME/.pyenv"
		PATH="$PYENV_ROOT/bin:$PATH"
		eval "$(pyenv init -)"
		
		pyenv install -s ${PYTHON_VERSION_MODERN}
		pyenv global ${PYTHON_VERSION_MODERN}
		
		pyenv rehash
		python -m pip install --user virtualenv		

		python -m virtualenv ~/.venv
		source ~/.venv/bin/activate
		
		# gives error 9.11.2017
		#pip install --upgrade pip
		pip install -q numpy
		
	else
		export ROOT_FILENAME=${ROOT5_FILENAME_MAC}
		# install pyenv
		# https://github.com/pyca/cryptography/blob/master/.travis/install.sh
		git clone https://github.com/yyuu/pyenv.git ~/.pyenv
		PYENV_ROOT="$HOME/.pyenv"
		PATH="$PYENV_ROOT/bin:$PATH"
		eval "$(pyenv init -)"
		
		pyenv install -s ${PYTHON_VERSION_OLD}
		pyenv global ${PYTHON_VERSION_OLD}
		
		pyenv rehash
		python -m pip install --user virtualenv		

		python -m virtualenv ~/.venv
		source ~/.venv/bin/activate
		
		# gives error 9.11.2017
		#pip install --upgrade pip
		pip install -q numpy		
	fi
	
else
	if [[ $OPTION == 'modern' ]]; then
		
		export PIP_REQUIRE_VIRTUALENV=true
		
		pyenv install -s ${PYTHON_VERSION_MODERN}
		pyenv global ${PYTHON_VERSION_MODERN}
		pyenv versions
		
		git clone https://github.com/yyuu/pyenv-pip-rehash.git ~/.pyenv/plugins/pyenv-pip-rehash
		git clone https://github.com/yyuu/pyenv-virtualenv.git ~/.pyenv/plugins/pyenv-virtualenv
		#eval "$(pyenv init -)"
		#eval "$(pyenv virtualenv-init -)"
		pyenv init
		pyenv virtualenv-init
		#pyenv virtualenv-init -
		
		pyenv virtualenv ${PYTHON_VERSION_MODERN} my-virtual-env
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

		pyenv install -s ${PYTHON_VERSION_OLD}
		pyenv global ${PYTHON_VERSION_OLD}
		pyenv versions
		
		git clone https://github.com/yyuu/pyenv-pip-rehash.git ~/.pyenv/plugins/pyenv-pip-rehash
		git clone https://github.com/yyuu/pyenv-virtualenv.git ~/.pyenv/plugins/pyenv-virtualenv
		pyenv init
		pyenv virtualenv-init
		#pyenv virtualenv-init -
		
		pyenv virtualenv ${PYTHON_VERSION_OLD} my-virtual-env
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
		
fi
	
echo "Python has been installed"