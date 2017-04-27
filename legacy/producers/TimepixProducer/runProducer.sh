#!/bin/bash

source setup_producer.sh

export RUNCONTROL="tcp://192.168.1.2:44000"

echo "###############################################" >> logs/producer_log.txt
date >> logs/producer_log.txt
echo "Started Producer" >> logs/producer_log.txt
echo "################ execution log ################" >> logs/producer_log.txt

cd $TPPROD/Pixelman_SCL_2011_12_07
eval "./TimepixProducer.exe -r $RUNCONTROL"
cd $TPPROD
