
import sys, json, datetime


def checkEvNo(f):
    prev = -1
    data = False
    evNo = False
    while f.readNext() :
        data = json.loads( f.getJsonPacketInfo() )
        for meta in data['meta'] :
            if meta['type'] == 4:
                evNo = meta
                if prev == -1 :
                    print 'first: {0} in packet# {1}'.format( meta['counter'], data['header']['packetNumber'] )
                elif meta['counter'] != prev + 1:
                    print 'in packet# {0}: ev# = {1}, expected: {2}'.format( data['header']['packetNumber'], meta['counter'], prev + 1 )
                prev = meta['counter']    
    print 'last: {0} in packet# {1}'.format( evNo['counter'], data['header']['packetNumber'] )



def next(f):
    if f.readNext() :
        return json.loads( f.getJsonPacketInfo() )
    return False

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




sys.path.append( '../../lib/' )
from libPyAidaFileReader import *
fileName = sys.argv[1]
f = eudaq.AidaFileReader( fileName )
config = json.loads( f.getJsonConfig() )
print 'run# {0} started at {1}'.format( config['runnumber'], config['date'] )
printMeta(f)


