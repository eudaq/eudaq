#! /usr/bin/env python
# load binary lib/pyeudaq.so
# Mengqing Wu <mengqing.wu@desy.de>
# created @ Jun
# 1st stable version out on July 20, 2019

import os, sys, datetime, time

import pyeudaq

import pyrogue
import rogue

import os
kpixdir=os.environ['KPIX_DIR']

pyrogue.addLibraryPath(kpixdir+'/python')
import KpixDaq

sys.path.append('./')
from _EuDataTools import EuDataStream


class kpixPyProducer(pyeudaq.Producer):
    def __init__(self, name, runctrl):
        print("__init__")
        
        pyeudaq.Producer.__init__(self, 'PyProducer', name, runctrl)
        self.is_running = 0
        print ('New instance of kpixPyProducer')
                
        self.ip = '192.168.2.10'
        self.debug = False

        self.kpixconf = None
        self.outfile = None
        self.runcount = None
        self.root = None
        self.triggermode = False

    def __enter__(self):
        print("__enter__")
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        print("__exit__")
                
    def DoTerminate(self):
        """ Essencial to terminate KPiX class object correctly. """
        print ('DoTerminate')
            
    def DoStatus(self):
        #print ('DoStatus')
        stop = self.is_running and "false" or "true"
        pyeudaq.Producer.SetStatusTag(self,"KpixStopped", stop);
        
    def DoInitialise(self):        
        print ('DoInitialise')
        print ('kpix py producer do nothing here.')
        
    def DoConfigure(self):
        print ('DoConfigure')

        if self.GetConfigItem("KPIX_CONF_FILE"):
            self.kpixconf= self.GetConfigItem("KPIX_CONF_FILE")
        
        if self.GetConfigItem("RUN_COUNT"):
            self.runcount= int(self.GetConfigItem("RUN_COUNT"))

        if self.GetConfigItem("KPIX_TRIGGER_MODE"):
            self.triggermode= self.GetConfigItem("KPIX_TRIGGER_MODE")
        
        kpixout = self.GetConfigItem("KPIX_OUT_FILE")
        if os.path.isdir(kpixout):
            self.outfile = os.path.abspath(datetime.datetime.now().strftime(f"{kpixout}/Run_%Y%m%d_%H%M%S.dat"))
        else:
            self.outfile = os.path.abspath(datetime.datetime.now().strftime(f"/scratch/data/Run_%Y%m%d_%H%M%S.dat"))
        print (f"write kpix outfile to {self.outfile}")

        if self.root:
            print (" I do have DESY root opened already")
        else:
            print ("No DESY root currently opened.")

    def DoStartRun(self):
        print ('DoStartRun')
        self.is_running = 1

        
    def DoStopRun(self):        
        print ('DoStopRun')
        self.root.DesyTrackerRunControl.runState.setDisp('Stopped')
        self.root.DataWriter.open.set(False)
        self.is_running=0
        print(f'DoStopRun: Ending Run')
            
    def DoReset(self):        
        print ('DoReset')
        if self.root:
            self.root.DesyTrackerRunControl.runState.setDisp('Stopped')
            self.root.DataWriter.open.set(False)
            print(f'DoReset: Ending Run')
 
        self.is_running = 0
        
    def RunLoop(self):
        print ("Start of RunLoop in kpixPyProducer")

        with KpixDaq.DesyTrackerRoot(pollEn=False, ip=self.ip, debug=self.debug) as root:
            print (root)
            self.root = root
            
            if self.root:
                print (" runloop : I do have DESY root opened now")
                print (self.root)
            else:
                print ("No DESY root currently opened.")
            
            print ('Reading all')
            root.ReadAll()
            root.waitOnUpdate()

            # Print the version info
            root.DesyTracker.AxiVersion.printStatus()

            #fi     
            root.DataWriter.dataFile.setDisp(self.outfile)
            root.DataWriter.open.set(True)
            print(f'Opening data file: {self.outfile}')
            
            print("Hard Reset")
            root.HardReset()
            print("Count Reset")
            root.CountReset()

            print('Writing initial configuration')
            root.LoadConfig(self.kpixconf)
            root.ReadAll()
            root.waitOnUpdate()
            
            print (f"KPiX configured from {self.kpixconf}")
            if self.runcount:
                root.DesyTrackerRunControl.MaxRunCount.set(self.runcount)

            # connect kpix udp data stream to EUDAQ @ Jun 18
            dataline = root.udp.application(dest=1)
            #fp = KpixDaq.KpixStreamInfo() # mq added
            fp = EuDataStream() # mq 
            fp.setEudaq(self)
            pyrogue.streamTap(dataline, fp)
            
            root.DesyTrackerRunControl.runState.setDisp('Running')
            root.DesyTrackerRunControl.waitStopped()
            root.DataWriter.open.set(False)
            self.is_running=0
            print(f'runloop: Ending Run')
      

            print ("Ended: kpixDAQ root is going to be killed.")

            print ("self.root will be None.")
            self.root = None
            
        # fi- with statement
            
        print ("End of RunLoop in kpixPyProducer")


if __name__ == "__main__":
    # #    myproducer= kpixPyProducer("newkpix", "tcp://192.168.200.1:44000")
    with kpixPyProducer("newkpix", "tcp://localhost:44000") as myproducer:
        print ("connecting to runcontrol in localhost:44000" )

        myproducer.Connect()
        time.sleep(2)

        while(myproducer.IsConnected()):
            time.sleep(1)
