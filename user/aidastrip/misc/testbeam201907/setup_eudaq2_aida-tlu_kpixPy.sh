# IP variables for start scripts
echo "Setup IP for RunControl, TLU, and NI as : localhost..."
export RUNCONTROLIP=127.0.0.1
export TLUIP=127.0.0.1
export NIIP=127.0.0.1
#export NIIP=192.168.200.101

echo "Check pathes in telescope.ini and in the conf-files"
# eudaq2 binaries
export EUDAQ=/opt/eudaq2/
export PATH=$PATH:$EUDAQ/bin

echo "Setup AIDA TLU environment..."
## for AIDA TLU
# for Ubuntu 18.04
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/cactus/lib
# start controlhub for stable AIDA TLU TCP/IP communication
/opt/cactus/bin/controlhub_start
/opt/cactus/bin/controlhub_status

#echo "Setup KPiX Py environment..."
#source ${EUDAQ}/user/aidastrip/python/setup_eudaq_kpixRogue.sh
