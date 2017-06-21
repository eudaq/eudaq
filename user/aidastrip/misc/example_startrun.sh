#!/usr/bin/env sh
./euRun -a tcp://44000 &
#./euCliMonitor -n Ex0Monitor -t mon
#./euCliCollector -n Ex0TgDataCollector -t tg
#./euCliProducer -n Ex0Producer -t p0
./euCliProducer -n myEx0Producer -t p1
./euCliProducer -n kpixProducer -t pp
