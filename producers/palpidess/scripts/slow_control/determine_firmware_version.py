#! /usr/bin/env python
##
## reading the firmware version from the FEC and print it
##
import sys
import SlowControl # slow control code

if (len(sys.argv) < 2):
    print "ERROR: please specific FEC ID"
    quit(1)

s = SlowControl.SlowControl(int(sys.argv[1]))
# GIT information of the firmware is located in the system register (port 6007)
# add the addresses 15 to 23
rply = SlowControl.read_burst(s, 6007, 15, 9)

# check whether the reply
if len(rply.data) != 9 or rply.success == False:
    print "ERROR: read request was not successful"
    print str(rply)
    quit(2)
else:
    print "FEC_VERSION=0x%x" % rply.data[0]
    print "GIT_HASH=0x%0.8x%0.8x%0.8x%0.8x%0.8x" % \
          ( rply.data[5], rply.data[4], rply.data[3], rply.data[2],
            rply.data[1] )
    print "GIT_HASH_ABBR=0x%0.7x" % (rply.data[5] >> 4)
    print "BUILD_DATE=%0.8x" % rply.data[6]
    print "BUILD_TIME=%0.4x" % rply.data[7]
    print "BUILD_MOD=%d" % rply.data[8]

    print ""
    print str(rply)
    quit(0)
