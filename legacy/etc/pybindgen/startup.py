
import sys, json
sys.path.append( '../../lib/' )
from libPyIndexReader import *
from libPyAidaFileReader import *
fileName = sys.argv[1]
if fileName.endswith( 'raw2' ) :
    f = eudaq.AidaFileReader( fileName )
else :
    f = eudaq.IndexReader( fileName )
config = json.loads( f.getJsonConfig() )
print 'run# {0} started at {1}'.format( config['runnumber'], config['date'] )


