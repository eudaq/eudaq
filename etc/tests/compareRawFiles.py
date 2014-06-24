#!/usr/bin/env python2
"""
A small tool to compare two EUDAQ (binary) RAW files based on their MD5 sum

The (unencoded) date string found in the run header is removed via a regular expression to allow the comparison between files recorded at different times but where the same result is expected.

Intended for automated regression tests.

"""
import sys

def main(argv=None):
    """  main routine of compareRawFiles: a tool for EUDAQ binary RAW file comparisons """
    import os.path
    import argparse
    # Import regular expression library
    import re

    if argv is None:
        argv = sys.argv
        progName = os.path.basename(argv.pop(0))

    # command line argument parsing
    parser = argparse.ArgumentParser(prog=progName, description="A tool for comparing EUDAQ (binary) raw files based on MD5 sums and ignoring the (unencoded) time stamp in the run header.")
    parser.add_argument("-p", "--pattern", default="run$6R.raw", help="If the first argument is a directory, this pattern will be used together with the information from the 'runnumber.dat' file to construct the file name for the last written run to compare against.", metavar="PATTERN")
    parser.add_argument("testfile", help="The raw file to test.")
    parser.add_argument("referencefile", help="The reference file to compare against.")
    args = parser.parse_args(argv)
    
    # if the testfile we were given is actually a directory, we determine the last run from the
    # EUDAQ 'runnumber.dat' file and use that as file to test against
    if os.path.isdir(args.testfile):
        print "First argument is a directory, will determine last run from 'runnumber.dat' file"
        if os.path.exists(os.path.join(args.testfile,"runnumber.dat")):
            runnumbers = []
            f = open(os.path.join(args.testfile,"runnumber.dat"), 'r')
            try:
                lines = f.read().splitlines()
                for line in lines:
                    runnumbers.append(line)
            finally:
                f.close()
            if len(runnumbers) != 1:
                print "Error: Found too many/few lines in 'runnumber.dat' file!"
                return 1
            try:
                run = int(runnumbers[0])
            except ValueError:
                print "Error: Runnumber in 'runnumber.dat' is not an integer!"
                return 1
            # determine amount of padding from pattern argument
            p = re.compile("\$[0-9]R") # the key pattern determining the amount of zero padding of the run number in the file name
            x = re.compile("\$X") # the extension pattern in case the full EUDAQ file naming scheme is used
            m = p.search(args.pattern)
            if not m:
                print "Error: provided file pattern does not contain the key '$XR' where X is a integer determining the amount of padding"
                return 1
            padding = int(m.group()[1]) # second char is the integer for the padding

            # now actually construct the file name: search and replace pattern key with padded run number
            m = p.subn(str(run).zfill(padding), args.pattern, count=1)
            m = x.subn(".raw", m[0], count=1) # replace a possible occurance of the file extension pattern with "raw"
            t = m[0]
            t = os.path.join(args.testfile, t) # append the path
        else:
            print "Error: Cannot find 'runnumber.dat' in path '"+args.testfile+"'"
            return 1
    else:
        # testfile is an actual file
        t = args.testfile
    
    # Import hashlib library (md5 method is part of it)
    import hashlib
    # we are looking for strings such as 2013-12-05 19:54:22.590
    dates = re.compile("[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}")

    refdate = "1990-06-06 12:34:27.170" # this will be used as replacement for the first date matching above regex
    
    # Open,close, read file and calculate MD5 on its contents
    with open(t, 'r') as file_to_check:
        # read contents of the file
        data = file_to_check.read()
        # replace first time stamp match to effectively remove it from the comparison
        m = dates.subn(refdate, data, count=1)
        #print "Number of time stamps replaced in the run header of the test file: " + str(m[1])
        # pipe contents of the file through
        md5_testfile = hashlib.md5(m[0]).hexdigest()    
    
    with open(args.referencefile, 'r') as file_to_check:
        # read contents of the file
        data = file_to_check.read()
        # replace first time stamp match to effectively remove it from the comparison
        m = dates.subn(refdate, data, count=1)
        #print "Number of time stamps replaced in the run header of the reference file: " + str(m[1])
        # pipe contents of the file through
        md5_reference = hashlib.md5(m[0]).hexdigest()

    if md5_reference == md5_testfile:
        print "Both files have the same MD5 sum: " + md5_testfile
    else:
        print "MD5 sum verification failed -- file contents are different!"
        print t + ": " + md5_testfile
        print args.referencefile + ": " + md5_reference
        return 1
    return 0
        
if __name__ == "__main__":
    sys.exit(main())
