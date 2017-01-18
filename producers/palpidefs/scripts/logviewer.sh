#!/bin/bash

LOGFILE=$1

cat $1 | grep -v WARN | grep -v Reading | grep -v mutex | grep -v Preparing | grep -v Reconfiguration | grep -v "End of run" | grep -v Stopping | grep -v "Moving" | grep -v "Event limit" | sed -r 's/^.{5}//' | cut -f 1- -d "/" | sed 's/RunControl//g' | sed 's/DataCollector//g' | sed 's/Configuring/\nConfiguring/g' | grep -v "Out of sync" | grep -v "Connection" | grep -v "started" | grep -v "Disconnected" | grep -v "Terminating" | grep -v "Out-of-sync" | while read l
do
#    echo $l
    printindex=0
#    if [ "${#l}" -eq 0 ]
#    then
#        echo -e $RUN_NUMBER'\t'$RUN_START_DATE'\t'$RUN_START_TIME'\t'$RUN_END_TIME'\t'$EVENT_COUNT'\t'$CONFIG_FILE
#        CONFIG_FILE="-"
#        RUN_NUMBER="-"
#        RUN_START_DATE="-"
#        RUN_START_TIME="-"
#        EVENT_COUNT="-"
#        RUN_END_DATE="-"
#        RUN_END_TIME="-"
#    fi
    if [[ $l == Configuring* ]]
    then
#        echo $l
        CONFIG_FILE=$(echo $l | cut -f2 -d"(" | cut -f1 -d")" | rev | cut -f 1 -d"/" | gawk '{ print gensub(/fnoc./, "", 1) }' | rev )".conf"
    fi
    if [[ $l == Starting\ Run* ]]
    then
        RUN_NUMBER=$(echo $l | cut -f3 -d' ' | sed 's/://')
        RUN_START_DATE=$(echo $l | cut -f4 -d' ')
        RUN_START_TIME=$(echo $l | cut -f5 -d' ' | cut -f1 -d'.')
        if [[ $RUN_START_DATE == Continued ]]
        then
            RUN_START_DATE=$(echo $l | cut -f5 -d' ' | cut -f1 -d'.')
            RUN_START_TIME=$(echo $l | cut -f6 -d' ' | cut -f1 -d'.')
        fi
#        echo $RUN_START_TIME 
    fi
    if [[ $l == *EORE* ]]
    then
        EVENT_COUNT=$(echo $l | cut -f5 -d' ')
        RUN_END_DATE=$(echo $l | cut -f6 -d' ')
        RUN_END_TIME=$(echo $l | cut -f7 -d' ' | cut -f1 -d'.')
#        echo $RUN_END_TIME
        printindex=1
    fi
    if [ "$printindex" -eq 1 ]
    then
        echo -e $RUN_NUMBER'\t'$RUN_START_DATE'\t'$RUN_START_TIME'\t'$RUN_END_TIME'\t'$EVENT_COUNT'\t'$CONFIG_FILE
        CONFIG_FILE="-"
        RUN_NUMBER="-"
        RUN_START_DATE="-"
        RUN_START_TIME="-"
        EVENT_COUNT="-"
        RUN_END_DATE="-"
        RUN_END_TIME="-"
    fi
done
