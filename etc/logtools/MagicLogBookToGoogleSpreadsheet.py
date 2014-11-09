#!/usr/bin/python

# Authors	  Mathieu Benoit (mbenoit@CERN.CH)
#		  Samir Arfaoui	  (sarfaoui@CERN.ch)

import time

#You need to have the gdata (google docs) libraries in your PYTHONPATH
import gdata.spreadsheet.service

import getpass
import os
import subprocess


#UGLY HACK, adding check_output for python version lacking it
if "check_output" not in dir( subprocess ): # duck punch it in!
    def f(*popenargs, **kwargs):
        if 'stdout' in kwargs:
            raise ValueError('stdout argument not allowed, it will be overridden.')
        process = subprocess.Popen(stdout=subprocess.PIPE, *popenargs, **kwargs)
        output, unused_err = process.communicate()
        retcode = process.poll()
        if retcode:
            cmd = kwargs.get("args")
            if cmd is None:
                cmd = popenargs[0]
            raise subprocess.CalledProcessError(retcode, cmd)
        return output
    subprocess.check_output = f




# Function parsing the magic log book for run times, number of events etc ...
def ParseMagicLogBook(FileLocation,runs) :
	
	cmd = "cd ~/workspace/eudaq/bin;./MagicLogBook.exe %s/run%s.raw -p full" % ( FileLocation, runs )
	result = subprocess.check_output(cmd, shell=True)
	
	datastr = result.split()[13:]
	
	print len(datastr) / 9
	nlines = len(datastr) / 9

	rundatalist = []	
	
	for i in range(nlines) : 
		l=datastr[i*9 :i*9+8]
		print datastr[i*9 :i*9+8]
		dico = {}
		dico['run'] = l[0]
		dico['time'] = l[2] + ' ' + l[3]
		dico['events'] = l[4]
		rundatalist.append(dico)
	return rundatalist



# Parsing of the logbook output for the files in your raw file folder, the run string support wildcards
#runlist = ParseMagicLogBook('/mnt/datura_raid/data/2014w6_CLIC','00319*')
runlist = ParseMagicLogBook('/Your/Raw/Folder','00*')



# Find this value in the google doc url with 'key=XXX' and copy XXX below
spreadsheet_key = '0AkFaasasfEWsddfegEWrgeRGergGRErgerGERG3c'
# All spreadsheets have worksheets. I think worksheet #1 by default always
# has a value of 'od6'
worksheet_id = 'od6'

spr_client = gdata.spreadsheet.service.SpreadsheetsService()
spr_client.email = 'yourusername'
spr_client.password = ' youruniquegeneratedpassword '
spr_client.source = 'Example Spreadsheet Writing Application'
spr_client.ClientLogin(spr_client.email,spr_client.password)

# Prepare the dictionary to write
# Columns are associated to dictionary keys, so you need to put a title to each to of column in your speadshoot
for dico in runlist : 
	entry = spr_client.InsertRow(dico, spreadsheet_key, worksheet_id)
	if isinstance(entry, gdata.spreadsheet.SpreadsheetsList):
		print "Insert row succeeded."
	else:
  		print "Insert row failed."


# Eh Voila you are done ! 
