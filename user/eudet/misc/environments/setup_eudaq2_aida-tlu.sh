# for Ubuntu 18.04
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/cactus/lib


# eudaq2 binaries
export PATH=$PATH:/opt/eudaq2/bin

# start controlhub for stable AIDA TLU TCP/IP communication
/opt/cactus/bin/controlhub_start
/opt/cactus/bin/controlhub_status
