#!/usr/bin/env sh
./euRun -n Ex0RunControl -a tcp://44000 &
sleep 1
./euLog -a tcp://44001&
sleep 1
./euCliMonitor -n Ex0Monitor -t my_mon  -a tcp://45001 &
./euCliCollector -n Ex0TgDataCollector -t my_dc -a tcp://45002 &
./euCliProducer -n Ex0Producer -t my_pd0 &
./euCliProducer -n Ex0Producer -t my_pd1 &
