#!/usr/bin/env sh                                                                                         
BINPATH=../../../bin
$BINPATH/euRun -n kpixRunControl &
sleep 1

python3 ExamplePyProducer.py
