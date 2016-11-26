#!/bin/bash

FILE_TO_UPLOAD_1=""
FILE_TO_UPLOAD_2=""

mkdir ~/packaging

if [[ $BUILD_manual == 'ON' ]]; then
	cp $TRAVIS_BUILD_DIR/eudaq/doc/manual/EUDAQUserManual.pdf ~/packaging/EUDAQUserManual-$TRAVIS_TAG.pdf
	export FILE_TO_UPLOAD_1="~/packaging/EUDAQUserManual-$TRAVIS_TAG.pdf"
else
	export FILE_TO_UPLOAD_1=""
fi