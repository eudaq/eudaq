#!/bin/bash     

## EUDAQ2 binaries:
export EUDAQ=/opt/eudaq2/
export PATH=$PATH:$EUDAQ/bin
echo "Setup EUDAQ path to" $EUDAQ

## KPiX Dir:
#export KPIX_DIR=/home/lycoris-dev/workspace/kpix.dev/software/
export KPIX_DIR=/home/lycoris-dev/workspace/kpix/software/
echo "Setup KPiX path to" $KPIX_DIR

## EUDAQ2 python libs:
echo "Setup EUDAQ python libs..."
export PYTHONPATH=${EUDAQ}/lib:${PYTHONPATH}
export LD_LIBRARY_PATH=${EUDAQ}/lib:${LD_LIBRARY_PATH}

## KPIX rogue python libs:
echo "Setup KPiX rogue python libs..."
if [ -z "$LD_LIBRARY_PATH" ]
then
   LD_LIBRARY_PATH=""
fi

if [ -z "$PYTHONPATH" ]
then
   PYTHONPATH=""
fi


#1 Setup Kpix, Rogue, and Surf DIRs
#export KPIX_DIR=/home/lycoris-admin/software/kpixDaq/kpix/software/
export ROGUE_DIR=${KPIX_DIR}/rogue/
export SURF_DIR=${KPIX_DIR}/../firmware/submodules/surf/

#2 Setup python path
export PYTHONPATH=${ROGUE_DIR}/python:${KPIX_DIR}/python:${SURF_DIR}/python:${PYTHONPATH}
export PYTHONPATH=${KPIX_DIR}/python:${SURF_DIR}/python:${PYTHONPATH}

#3 Setup library path
#export LD_LIBRARY_PATH=${ROGUE_DIR}/lib:${LD_LIBRARY_PATH}


# # Setup AIDA TLU on ubuntu 18.04
# echo "Setup AIDA TLU environment..."
# ## for AIDA TLU
# # for Ubuntu 18.04
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/cactus/lib
# # start controlhub for stable AIDA TLU TCP/IP communication 
# /opt/cactus/bin/controlhub_start
# /opt/cactus/bin/controlhub_status
