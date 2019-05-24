#!/bin/bash

# which subnet
RCPORT=44000
LOCAL=127.0.0.1
AIDALOCAL=192.168.200.1

if [ ! $1 ]; then
    echo "Please specify local subnet as first argument"
    echo "e.g. $LOCAL"
    echo "e.g. $AIDALOCAL"
    return
fi

# start EUDAQ RunControl and AidaTluProducer locally
../../../../bin/euRun &
sleep 1
../../../../bin/euCliProducer -n AidaTluProducer -t aida_tlu -r tcp://$1:$RCPORT &
