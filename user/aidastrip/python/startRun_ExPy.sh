#!/usr/bin/env sh                                                                                         
BINPATH=../../../bin
$BINPATH/euRun &
sleep 1

python3 ExamplePyProducer.py
