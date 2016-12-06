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

	echo "Setting C-Compiler to 4.9 for OSX"
	if [[ "$CC" == "gcc" ]]; then CC=gcc-4.9; fi	
	
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
			sudo apt-add-repository "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty main"
			sudo apt-get update
			sudo apt-get install clang-3.9 lldb-3.9
			#LLVM_DIR=${DEPS_DIR}/llvm-${LLVM_VERSION}
			#if [[ -z "$(ls -A ${LLVM_DIR})" ]]; then
			#	LLVM_URL="http://llvm.org/releases/${LLVM_VERSION}/llvm-${LLVM_VERSION}.src.tar.xz"
			#	LIBCXX_URL="http://llvm.org/releases/${LLVM_VERSION}/libcxx-${LLVM_VERSION}.src.tar.xz"
			#	LIBCXXABI_URL="http://llvm.org/releases/${LLVM_VERSION}/libcxxabi-${LLVM_VERSION}.src.tar.xz"
			#	CLANG_URL="http://llvm.org/releases/${LLVM_VERSION}/clang+llvm-${LLVM_VERSION}-x86_64-linux-gnu-ubuntu-14.04.tar.xz"
			#	mkdir -p ${LLVM_DIR} ${LLVM_DIR}/build ${LLVM_DIR}/projects/libcxx ${LLVM_DIR}/projects/libcxxabi ${LLVM_DIR}/clang
			#	travis_retry wget --quiet -O - ${LLVM_URL}      | tar --strip-components=1 -xJ -C ${LLVM_DIR}
			#	travis_retry wget --quiet -O - ${LIBCXX_URL}    | tar --strip-components=1 -xJ -C ${LLVM_DIR}/projects/libcxx
			#	travis_retry wget --quiet -O - ${LIBCXXABI_URL} | tar --strip-components=1 -xJ -C ${LLVM_DIR}/projects/libcxxabi
			#	travis_retry wget --quiet -O - ${CLANG_URL}     | tar --strip-components=1 -xJ -C ${LLVM_DIR}/clang
			#	(cd ${LLVM_DIR}/build && cmake .. -DCMAKE_INSTALL_PREFIX=${LLVM_DIR}/install -DCMAKE_CXX_COMPILER=clang++)
			#	(cd ${LLVM_DIR}/build/projects/libcxx && make install -j2)
			#	(cd ${LLVM_DIR}/build/projects/libcxxabi && make install -j2)
			#fi
			#export CXXFLAGS="-nostdinc++ -isystem ${LLVM_DIR}/install/include/c++/v1"
			#export LDFLAGS="-L ${LLVM_DIR}/install/lib -l c++ -l c++abi"
			#export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${LLVM_DIR}/install/lib"
			#export PATH="${LLVM_DIR}/clang/bin:${PATH}"
			export CXX="clang++" 
			export CC="clang"
		fi
    
	fi
	
fi    
	
echo "Current C++/C compilers after settings in set_compilers"

echo `$CXX --version`
echo `$CC --version`