#!/bin/bash

euRun &
sleep 2

euCliProducer -n WienerProducer -t wiener &
sleep 2

euCliCollector -n DirectSaveDataCollector -t wiener_dc &
