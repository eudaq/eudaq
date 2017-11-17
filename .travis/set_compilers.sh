#!/bin/bash

# partially taken from:
# https://github.com/boostorg/hana/blob/master/.travis.yml
# https://github.com/travis-ci/travis-ci/issues/6273
# https://github.com/travis-ci/travis-ci/issues/979#issuecomment-45740549
# https://github.com/travis-ci/travis-ci/issues/5959

echo "Entered set_compilers script"
echo "Compiler versions will be installed and set"
echo ""

echo "Current C++/C compilers"

echo `$CXX --version`
echo `$CC --version`

export DEPS_DIR=$HOME/dependencies

    
if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

	echo "Not setting a particular C++-Compiler for OSX"
	#if [[ "$CC" == "gcc" ]]; then CC=gcc-4.9; fi	
	
else	

	echo "Installing compiler for linux"
	
	if [[ $COMPILER == 'gcc' ]]; then
	
		echo "Installing g++ $GCC_VERSION for linux"
		sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
		sudo apt-get update
		sudo apt-get install -qq g++-$GCC_VERSION gcc-$GCC_VERSION
		export CXX="g++-$GCC_VERSION" 
		export CC="gcc-$GCC_VERSION"
		sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-$GCC_VERSION 90
		sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$GCC_VERSION 90
	
	else

		echo "Installing clang $LLVM_VERSION for linux"
		if [[ "${LLVM_VERSION}" != "" ]]; then
			wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
			sudo aptitude purge libclang-common-dev
			sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
			sudo apt-add-repository "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-$LLVM_VERSION main"
			sudo apt-get update
			# removing libllvm-$LLVM_VERSION-ocaml-dev  as 5.0 version cannot be found 9.11.2017
			sudo apt-get install g++-5 gcc-5 clang-$LLVM_VERSION clang-$LLVM_VERSION-doc libclang-common-$LLVM_VERSION-dev libclang-$LLVM_VERSION-dev libclang1-$LLVM_VERSION libclang1-$LLVM_VERSION-dbg libllvm$LLVM_VERSION libllvm$LLVM_VERSION-dbg liblldb-$LLVM_VERSION llvm-$LLVM_VERSION llvm-$LLVM_VERSION-dev llvm-$LLVM_VERSION-doc llvm-$LLVM_VERSION-examples llvm-$LLVM_VERSION-runtime clang-format-$LLVM_VERSION python-clang-$LLVM_VERSION liblldb-$LLVM_VERSION-dev lld-$LLVM_VERSION libfuzzer-$LLVM_VERSION-dev  -y
			export CXX="clang++-"$LLVM_VERSION 
			export CC="clang-"$LLVM_VERSION
		fi
    
	fi
	
fi    
	
echo "Current C++/C compilers after settings in set_compilers"

echo `$CXX --version`
echo `$CC --version`