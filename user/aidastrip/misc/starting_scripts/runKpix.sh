#!/bin/sh

EUDAQ=/home/lycoris-dev/eudaq/eudaq2.central/bin/
$EUDAQ/kpixGui -n kpixRunControl &

sleep 2

# Unless you do want to produce a eudaq raw file, turn on the DataCollector:
#./euCliCollector -n kpixDataCollector -t lycorisDC &

$EUDAQ/euCliProducer -n kpixProducer -t lycoris &
