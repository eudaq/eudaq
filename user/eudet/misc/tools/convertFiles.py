#!/usr/bin/python
# -*- coding: UTF-8 -*-

import re
import time
import os
import sys
from optparse import OptionParser
from os import listdir
from os.path import isfile, join

def multiplefiles(L):
    start, end = L[0], L[-1]
    return sorted(set(range(start, end + 1)).difference(L))

def getOptions():

	"""
	Parse and return the arguments provided by the user.
	"""
	usage = ("Usage: %prog --d <path with raw files>\n")
	parser = OptionParser(usage=usage)
        
	parser.add_option('--d', '--dir',
                      dest = 'dir',
                      default = '',
                      help = "Path with zip files",
                      metavar = '<path-raw-files>')

        (options, arguments) = parser.parse_args()

	if arguments:
		parser.error("Found positional argument(s): %s." % (arguments))
	if not options.dir:
        	parser.error("(--d <dir with raw files>, --dir=<dir with raw files>) option not provided.")

	return options

def main():

        listruns = []
        ordered = []
        i = 0
        for f in listdir(options.dir):
                if isfile(join(options.dir, f)):
                        if f.endswith(".raw"):

                            result = re.search('run(\d+)', f)
                            run_number = result.group(1)
                            run_number = run_number.replace("run","")
                            listruns.insert(i,int(run_number))
                            i = i + 1
                            #print run_number

        listruns.sort()
        ordered = multiplefiles(listruns)
        print listruns
        print ordered
                            

	for f in listdir(options.dir):
		if isfile(join(options.dir, f)):
			if f.endswith(".raw"):

				result = re.search('run(\d+)', f)
				run_number = result.group(1)	
				out = "run" + run_number + ".slcio"
				command = "euCliConverter -i " + options.dir + "/" + f + " " + options.dir + "/" + out

				file_tmp = options.dir + "/" + out
               			check_file = os.path.isfile(file_tmp)	

				if not check_file:
					print("File %s has been converted!", out)
                                        #print(command)
					#os.system(command)
				else:
					print("File already converted!", out)



if __name__ == '__main__':

	options = getOptions()
	main()

