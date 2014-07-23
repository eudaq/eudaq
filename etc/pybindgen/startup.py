
import sys, json

def printMeta( f ):
    while f.readNext() :
        data = json.loads( f.getJsonPacketInfo() )
        for meta in data['meta'] :
            print str(meta)

def tlu( f ):
    n = 0
    while f.readNext() :
        data = json.loads( f.getJsonPacketInfo() )
        n += 1
        for meta in data['meta'] :
            if meta['tlu'] :
                print 'packet# {0}: counter: {1}'.format( n, meta['counter'] )

def type1( f ):
    n = 0
    while f.readNext() :
        data = json.loads( f.getJsonPacketInfo() )
        n += 1
        for meta in data['meta'] :
            if meta['type'] == 1 and ( meta['counter'] != 1 ):
                print '{0} packet# {1}: counter: {2}'.format( data['header']['packetType'], n, meta['counter'] )

def type2( f ):
    n = 0
    while f.readNext() :
        data = json.loads( f.getJsonPacketInfo() )
        n += 1
        for meta in data['meta'] :
            if meta['type'] == 2 and ( meta['counter'] != 1 ):
                print 'packet# {0}: counter: {1}'.format( n, meta['counter'] )

def type3( f ):
    n = 0
    prev = 0
    while f.readNext() :
        data = json.loads( f.getJsonPacketInfo() )
        n += 1
        for meta in data['meta'] :
            if meta['type'] == 3:
                if prev == 0 :
                    print 'first packet# {0}: counter: {1}'.format( n, meta['counter'] )
                elif prev > meta['counter'] :
                    print 'packet# {0}: counter: {1}'.format( n, meta['counter'] )
                prev = meta['counter']    


def type4( f ):
    n = 0
    prev = 0
    while f.readNext() :
        data = json.loads( f.getJsonPacketInfo() )
        n += 1
        for meta in data['meta'] :
            if meta['type'] == 4:
                if prev == 0 :
                    print 'first packet# {0}: counter: {1}'.format( n, meta['counter'] )
                elif prev > meta['counter'] :
                    print 'packet# {0}: counter: {1}'.format( n, meta['counter'] )
                prev = meta['counter']    



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


