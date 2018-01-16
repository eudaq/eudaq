#!/usr/bin/env sh
./euRun -a tcp://44000 &
#./euCliMonitor -n Ex0Monitor -t mon
#./euCliCollector -n Ex0TgDataCollector -t tg
#./euCliProducer -n Ex0Producer -t p0
./euCliProducer -n myEx0Producer -t p1
./euCliProducer -n kpixProducer -t pp


exit
#--------------------------------------------------------------#
### NOTE: should not run following commands simply with bash ###
#--------------------------------------------------------------#

#==> Data Converter:
#from eudaq_dev:
     ./euCliConverter -i tb-kpix-Aug2017/kpix_output/170818135757_run000087.raw -o xxx.slcio
     ./euCliReader -e 11 -E 12 -i tb-kpix-Aug2017/kpix_output/170818135757_run000087.raw  -std
#from eudaq_kpix:
     ./euCliReader -e 11 -E 12 -i ../data/tb170818/kpix_output/170818150913_run000088.raw -std

#===> KPiX:

#aida2020-kpix1:~/afs/kpix_dev/bin> 
./analysis ../xml/cal-desy3_uwe.xml ../data/2017_07_26_14_09_54.bin

./bin/kpixGui32 xml/cal-desy3_uwe.xml 

#===> EUDAQ: 
./myeuRun -n kpixRunControl &
./euCliCollector -n kpixDataCollector -t lycorisDC &
./euCliProducer -n kpixProducer -t lycoris &

./euCliProducer -n tbscProducer -t tbsc &
./euCliCollector -n tbscDataCollector -t tbscDC &

#==> EUDAQ2 with Mimosa: @August 16 <==
/home/teleuser/eudaq2/none.ini
/home/teleuser/Desktop/PIConfig1.conf

./euRun &
./euCliProducer -n TluProducer -t TLU &
./euCliProducer -n NiProducer -t MimosaNI &
./euCliCollector -n Ex0TgTsDataCollector -t tluDC &

#======= EUDAQ1
./TLUControl.exe -a 1 -d 0
