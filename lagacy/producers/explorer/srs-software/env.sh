#!/bin/sh

# determine dir where the env script is located
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

#Eudaq-SRS
export PATHSRS=$DIR

# more sophisticated with new line character detection, but unnecessary
# trailing space:
#DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd && echo x)"
#DIR="${DIR%x}"

if [ -z $ROOTSYS ]
then
    echo "need ROOT for running"
    exit
fi


if [ -z "$PYTHONPATH" ] # is variable set?
then
    export PYTHONPATH=${DIR}/common # was unset
else
    export PYTHONPATH=$PYTHONPATH:${DIR}/common # was set -> append
fi

#echo "PYTHONPATH="$PYTHONPATH
