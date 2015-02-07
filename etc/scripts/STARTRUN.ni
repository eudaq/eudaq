#!/bin/bash


######################################################################
# NI Producer
###############
if [ -f "NiProducer.exe" ]
then
    printf '\033[22;33m\t  NiProducer for linux  \033[0m \n'
    echo xterm -sb -sl 1000 -geom 80x10-480-700 -T 'Ni Producer for Linux' -e ./NiProducer.exe -r tcp://$HOSTIP:$RCPORT >> start.log
    xterm -sb -sl 1000 -geom 80x10-480-700 -T 'Ni Producer for Linux' -e ./NiProducer.exe -r tcp://$HOSTIP:$RCPORT &
    sleep 1
else
    printf '\033[22;31m\t  NiProducer for linux not found!  \033[0m \n'
    echo 'Configure EUDAQ with the CMake option "-D BUILD_ni=ON" and re-run "make install" to install.'
fi

