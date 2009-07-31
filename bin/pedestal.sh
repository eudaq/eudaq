#!/bin/bash

SRCUSER=eudet
SRCHOST=eudetpc001
SCRIPTPATH=/RAID/ilc/Eutelescope/v00-00-08-plugin/pysub
ENVFILE=./build_env.sh
DATAPATH=/RAID/ilc/data
DESTUSER=eudet
DESTHOST=mvme6100
DESTPATH=eudaq/pedestal
LASTFILE=pedestal.dat

if [ $# != 1 ]; then
  echo "usage $0 runnumber"
  echo "Converts a run, calculates pedestals, uploads for use in the DAQ, and updates the latest pedestal setting"
  echo "You should make sure you have an ssh key set up to allow you to log into ${SRCUSER}@${SRCHOST} without a password"
  exit 1
fi

RUNNUM=$1
# Some zeroes, for padding
TMP=000000
# Zero-padded run number, for the pedestal file names
RUNNUM0=${TMP:0:$((${#RRUNNUM} > 6 ? 0 : 6 - ${#RUNNUM}))}${RUNNUM}

echo "Converting run ${RUNNUM}"
ssh -x ${SRCUSER}@${SRCHOST} "cd ${SCRIPTPATH} && source ${ENVFILE} && ./submit-converter.py ${RUNNUM}" || exit 1

echo "Calculating pedestals for run ${RUNNUM}"
ssh -x ${SRCUSER}@${SRCHOST} "cd ${SCRIPTPATH} && source ${ENVFILE} && ./submit-pedestal.py ${RUNNUM}" || exit 1

# Display some histograms here...
# Ask if they look OK...

echo "Copying pedestals for run ${RUNNUM}"
ssh -x ${SRCUSER}@${SRCHOST} "scp ${DATAPATH}/db/run${RUNNUM0}-ped-db-* ${DESTHOST}:${DESTPATH}/." || exit 1

echo "Updating latest pedestal info"
# Do it via $SRCHOST, because we know ssh keys are set up from there
echo ${RUNNUM} | ssh -x ${SRCUSER}@${SRCHOST} "ssh -x ${DESTUSER}@${DESTHOST} tee ${DESTPATH}/${LASTFILE}" || exit 1

echo "OK"
