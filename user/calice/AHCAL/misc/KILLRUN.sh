#!/bin/bash
# cleanup of DAQ
PROCESSES="euRun euLog euCliCollector euCliProducer euCliMonitor StdEventMonitor AHCALProducer"
for thisprocess in $PROCESSES
do
    echo "Checking if $thisprocess is running... "
    pid=`pgrep -f $thisprocess`
    for i in $pid
    do 
     if [ $i ]
     then
	echo "Killing $thisprocess with pid: $pid"
        killall  $thisprocess 
	kill -9 $pid
     fi
    done
done
#example producer locks
test -f /tmp/mydev0.lock && rm /tmp/mydev0.lock
test -f /tmp/mydev1.lock && rm /tmp/mydev1.lock

# exit with status message if any process has been killed
printf '\033[1;32;48mKILLING DAQ COMPLETE, type STARTRUN to relaunch\033[0m \n'
