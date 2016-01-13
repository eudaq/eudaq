#!/bin/bash

export RCPORT=44000
[ "$1" != "" ] && RCPORT=$1

export HOSTIP=127.0.0.1
export TLUIP=127.0.0.1
export HOSTNAME=127.0.0.1

#cd `dirname $0`
#export LD_LIBRARY_PATH="`pwd`/bin:$LD_LIBRARY_PATH"

export EUDAQDIR=$PWD/../../../
export EUDAQBIN=$EUDAQDIR/bin
export CALEUDAQ=$PWD
export DATADIR=$EUDAQDIR/data

#done in the /etc/bashrc file
#check it!
export PATH=$PATH:$EUDAQDIR/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$EUDAQDIR/lib
