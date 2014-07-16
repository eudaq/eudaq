
import sys, json
sys.path.append( '/home/behrens/git/eudaq_build/main/python/' )
from libPyIndexReader import *
f = eudaq.IndexReader( '../data/run000012.idx' )
config = json.loads( f.getJsonConfig() )
print 'run# {0} started at {1}'.format( config['runnumber'], config['date'] )


