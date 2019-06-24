#!/usr/bin/env sh
BINPATH=../../../bin
#$BINPATH/euRun &
$BINPATH/euRun -n Ex0RunControl &
sleep 1
$BINPATH/euLog &
sleep 1
$BINPATH/euCliProducer -n PIStageProducer -t Stage &
