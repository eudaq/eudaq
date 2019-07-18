#! /usr/bin/env python
# load binary lib/pyeudaq.so
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
        self.root = None

    def __enter__(self):
        print("__enter__")
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        print("__exit__")
                
    def DoTerminate(self):
        """ Essencial to terminate KPiX class object correctly. """
        print ('DoTerminate')
        if self.root:
            self.root.stop()
        
    def DoStatus(self):
        #print ('DoStatus')
        stop = self.is_running and "false" or "true"
        pyeudaq.Producer.SetStatusTag(self,"KpixStopped", stop);
        
    def DoInitialise(self):        
        print ('DoInitialise')
        #print 'key_a(init) = ', self.GetInitItem("key_a")
        #-- start of set up desytrackerroot obj
        if not self.root:
            print (' MQ: init kpix root...')
            self.root= KpixDaq.DesyTrackerRoot(pollEn=False, ip=self.ip, debug=self.debug) 

            print ('Reading all')
            self.root.ReadAll()
            self.root.waitOnUpdate()
            # Print the version info
            self.root.DesyTracker.AxiVersion.printStatus()
        #-- end of set up desytrackerroot obj

    def DoConfigure(self):
        print ('DoConfigure')
               
        self.root.DesyTrackerRunControl.runRate.setDisp('Auto')

        #print 'key_b(conf) = ', self.GetConfigItem("key_b")
        kpixconf= self.GetConfigItem("KPIX_CONF_FILE")
        kpixout = self.GetConfigItem("KPIX_OUT_FILE")
        runcount= int(self.GetConfigItem("RUN_COUNT"))

        print(f"Hard Reset")
        self.root.HardReset()
        print(f"Count Reset")        
        self.root.CountReset()
        
        print (f"KPiX configured from {kpixconf}")
        self.root.LoadConfig(kpixconf)

        self.root.ReadAll()
        self.root.waitOnUpdate()

        if runcount != 0:
            self.root.DesyTrackerRunControl.MaxRunCount.set(runcount)
        if os.path.isdir(kpixout):
            outfile = os.path.abspath(datetime.datetime.now().strftime(f"{kpixout}/Run_%Y%m%d_%H%M%S.dat") )
            print (f"write kpix outfile to {outfile}")
            self.root.DataWriter.dataFile.setDisp(outfile)
            self.root.DataWriter.open.set(True)

        # start experiments to get data streams connected @ Jun 18
        dataline = self.root.udp.application(dest=1)
        #fp = KpixDaq.KpixStreamInfo() # mq added
        fp = EuDataStream() # mq 
        fp.setEudaq(self)
        pyrogue.streamTap(dataline, fp)
            
    def DoStartRun(self):
        # print(f"Hard Reset")
        # self.root.HardReset()
        # time.sleep(.2)
        
        # print(f"Count Reset")   
        # self.root.CountReset()
        # time.sleep(.2)

        #self.root.DesyTrackerRunControl.runCount.set(0)

        print ('DoStartRun')
        self.is_running = 1

        
    def DoStopRun(self):        
        print ('DoStopRun')
        self.is_running = 0
        self.root.DesyTrackerRunControl.runState.setDisp('Stopped')
        self.root.DataWriter.open.set(False)
                    
        print (' MQ: stop DesyTrackerRunControl')
        #self.root.stop() # TODO: if the stop() works properly with new kpix daq commit, uncomment here
         
    def DoReset(self):        
        print ('DoReset')
        self.is_running = 0
        self.root.HardReset()
        self.root.CountReset()
        #self.root.stop()

    def RunLoop(self):
        print ("Start of RunLoop in kpixPyProducer")
        
        try:

            self.root.DesyTrackerRunControl.runState.setDisp('Running')
            self.root.DesyTrackerRunControl.waitStopped()
        except(KeyboardInterrupt or not self.is_running):
            self.root.DesyTrackerRunControl.runState.setDisp('Stopped')
        self.is_running=False
        # while (self.is_running):
        #     self.root.DesyTrackerRunControl.waitStopped()
        #     self.root.DesyTrackerRunControl.runState.setDisp('Stopped')
        #     self.is_running = False
        #     time.sleep(1)

        # end of while loop
        #self.is_running=False
        print ("End of RunLoop in kpixPyProducer")


if __name__ == "__main__":
    # #    myproducer= kpixPyProducer("newkpix", "tcp://192.168.200.1:44000")
    with kpixPyProducer("newkpix", "tcp://localhost:44000") as myproducer:
        print ("connecting to runcontrol in localhost:44000" )

        myproducer.Connect()
        time.sleep(2)

        while(myproducer.IsConnected()):
            time.sleep(1)
