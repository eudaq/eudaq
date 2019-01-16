#!/usr/bin/env sh                                                                                         
BINPATH=../../../bin
$BINPATH/euRun &
sleep 10

python3 ExamplePyProducer.py &
python3 ExamplePyDataCollector.py &
