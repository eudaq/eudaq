#!/usr/bin/env sh                                                                                         
export KPIX_DIR=/home/lycoris-admin/software/kpixDaq/kpix_20181127/software/

export ROGUE_DIR=${KPIX_DIR}/rogue/

export SURF_DIR=${KPIX_DIR}/../firmware/submodules/surf/
       

export PYTHONPATH=${ROGUE_DIR}/python:${KPIX_DIR}/python:${SURF_DIR}/python:${PYTHONPATH}
export LD_LIBRARY_PATH=${ROGUE_DIR}/lib:${LD_LIBRARY_PATH}
