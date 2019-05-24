#!/bin/bash

# Add Cactus library, e.g. needed for Ubuntu 18.04
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/cactus/lib
# start controlhub for AIDA TLU TCP/IP communication
/opt/cactus/bin/controlhub_start
/opt/cactus/bin/controlhub_status
