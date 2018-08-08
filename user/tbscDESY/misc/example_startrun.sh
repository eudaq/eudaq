#!/usr/bin/env sh
#./euRun  &
#./euCliMonitor -n Ex0Monitor -t mon
#./euCliCollector -n Ex0TgDataCollector -t tg
#./euCliProducer -n Ex0Producer -t p0

#--------------------#
### Example to run ###
#--------------------#
#===> EUDAQ2 with tbsc modules 
./euCliProducer -n tbscProducer -t tbsc &
./euCliCollector -n tbscDataCollector -t tbscDC &

exit
#-----------------------------------------------#
## Examples on how to analyse the output data: ##
#-----------------------------------------------#
#==> w/ Data Converter:
# to convert into a csv file:
./tbscCliConverter -i tbsc02_180126175651_run000071.raw -o test.csv
# to read out certain event(s)
./euCliReader -e 11 -E 12 -i tbsc02_180126175651_run000071.raw -std
