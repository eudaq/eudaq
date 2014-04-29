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

    if argv is None:
        argv = sys.argv
        progName = os.path.basename(argv.pop(0))

    # command line argument parsing
    parser = argparse.ArgumentParser(prog=progName, description="A tool for comparing EUDAQ (binary) raw files based on MD5 sums and ignoring the (unencoded) time stamp in the run header.")
    parser.add_argument("testfile", help="The raw file to test.")
    parser.add_argument("referencefile", help="The reference file to compare against.")
    args = parser.parse_args(argv)

    # Import hashlib library (md5 method is part of it)
    import hashlib
    # Import regular expression library
    import re

    # we are looking for strings such as 2013-12-05 19:54:22.590
    dates = re.compile("[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}")

    refdate = "1990-06-06 12:34:27.170" # this will be used as replacement for the first date matching above regex
    
    # Open,close, read file and calculate MD5 on its contents
    with open(args.testfile, 'r') as file_to_check:
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
        print args.testfile + ": " + md5_testfile
        print args.referencefile + ": " + md5_reference
        return 1
    return 0
        
if __name__ == "__main__":
    sys.exit(main())
