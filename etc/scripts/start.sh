#!/bin/bash

export RCPORT=44000
[ "$1" != "" ] && RCPORT=$1

export HOSTIP=127.0.0.1
export TLUIP=127.0.0.1
export HOSTNAME=127.0.0.1

cd `dirname $0`
export LD_LIBRARY_PATH="`pwd`/lib:$LD_LIBRARY_PATH"

printf '\033[1;32;48m \t STARTING DAQ LOCALLY\033[0m \n'
echo $(date)
printf '\033[22;33m\t Cleaning up first...  \033[0m \n'

if [ -f KILLRUN.local ]
then
    sh KILLRUN.local
else
    sh KILLRUN
fi

printf '\033[22;31m\t End of killall \033[0m \n'

sleep 1

######################################################################
if [ -n "`ls data/run*.raw`" ]
then
    printf '\033[22;33m\t Making sure all data files are properly writeprotected \033[0m \n'
    chmod a=rw data/run*.raw
    printf '\033[22;32m\t ...Done!\033[0m \n'
fi

cd bin/

../STARTRUN.local
../STARTRUN.exampleProducer2
../STARTRUN.ni
../STARTRUN.tlu
../STARTRUN.onlinemon


cd ../


printf ' \n'
printf ' \n'
printf ' \n'
printf '\033[1;32;48m\t ...Done!\033[0m \n'
printf '\033[1;32;48mSTART OF DAQ COMPLETE\033[0m \n'
