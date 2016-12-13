#!/bin/bash

FILE_TO_UPLOAD_1=""
FILE_TO_UPLOAD_2=""

mkdir ${HOME}/packaging

if [[ $BUILD_manual == 'ON' ]]; then
	cp $TRAVIS_BUILD_DIR/doc/manual/EUDAQUserManual.pdf ${HOME}/packaging/"EUDAQUserManual-$TRAVIS_TAG.pdf"
	export FILE_TO_UPLOAD_1="${HOME}/packaging/EUDAQUserManual-$TRAVIS_TAG.pdf"
	ls ${HOME}/packaging
	ls $FILE_TO_UPLOAD_1
else
	export FILE_TO_UPLOAD_1=""
fi