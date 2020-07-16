'''
@ Jun 19
MQ: moved to EUDAQ2 python dir;
@ Jun 18
MQ: created, needs to move to EUDAQ2 dir;
'''

import sys

import pyrogue
import rogue

import os
kpixdir=os.environ['KPIX_DIR']

pyrogue.addLibraryPath(kpixdir+'/python')
import KpixDaq

import pyeudaq 
#eudaqdir = '/opt/eudaq2/user/aidastrip/python'
#pyrogue.addLibraryPath(eudaqdir)


class EuDataStream(rogue.interfaces.stream.Slave):
    def __init__(self):
        rogue.interfaces.stream.Slave.__init__(self)
        self.eudaq = None

    def setEudaq(self, eudaq): # mq
        self.eudaq = eudaq
       
    
    def _acceptFrame(self, frame):
       if frame.getError():
            print('Frame Error!')
            return

       ba = bytearray(frame.getPayload())
       frame.read(ba, 0)
       if frame.getChannel() == 0:
           #print(f'Got Frame: {len(ba)} bytes') # mengqing 
           d = KpixDaq.parseFrame(ba)

           # mq start here
           if self.eudaq:
               #print (' --Connected with EUDAQ side:')

               #- new a eudaq raw Event:
               ev = pyeudaq.Event("RawEvent", "KpixRawEvent") # to distinguish from the old kpix DAQ 'KpixRawEvt'

               #- Set EventN as Kpix
               ev.SetEventN(d['eventNumber'])
               
               #- Set TimeStamp(ns) as Kpix Shutter Timestamp per cycle
               ev.SetTimestamp( d['runtime']*5, d['runtime']*5+5, False) # end time += 1*runtime cycle
               
               ev.SetTag("Stamp","fromEuDataTools") # to remove
               #ev.SetTag("triggermode", self.eudaq.triggermode)
               
               block = bytes(ba)  # tentative mq, to verify
               ev.AddBlock(0, block ) # tentative mq, to verify
               self.eudaq.SendEvent(ev)

            # mq finish here
           

