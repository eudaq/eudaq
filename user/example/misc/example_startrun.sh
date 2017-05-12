#!/usr/bin/env sh
./euRun
./euCliMonitor -n Ex0Monitor -t mon
./euCliCollector -n Ex0TgDataCollector -t tg
./euCliProducer -n Ex0Producer -t p0
./euCliProducer -n Ex0Producer -t p1
