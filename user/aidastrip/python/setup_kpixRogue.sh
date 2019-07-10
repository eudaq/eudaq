#!/bin/bash

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
export KPIX_DIR=/home/lycoris-dev/workspace/kpix/software/
export ROGUE_DIR=${KPIX_DIR}/rogue/
export SURF_DIR=${KPIX_DIR}/../firmware/submodules/surf/

#2 Setup python path
export PYTHONPATH=${ROGUE_DIR}/python:${KPIX_DIR}/python:${SURF_DIR}/python:${PYTHONPATH}

#3 Setup library path
export LD_LIBRARY_PATH=${ROGUE_DIR}/lib:${LD_LIBRARY_PATH}
