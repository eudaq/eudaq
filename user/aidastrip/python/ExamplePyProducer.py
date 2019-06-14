#! /usr/bin/env python
# load binary lib/pyeudaq.so
import pyeudaq
import time
import pyrogue
import rogue

#import argparse

kpixdir='/home/lycoris-admin/software/kpixDaq/kpix/software'
pyrogue.addLibraryPath(kpixdir+'/python')
import KpixDaq


class ExamplePyProducer(pyeudaq.Producer):
    def __init__(self, name, runctrl):
        pyeudaq.Producer.__init__(self, 'PyProducer', name, runctrl)
        self.is_running = 0
        print ('New instance of ExamplePyProducer')

        print ('mq: init kpix root...')

        ip = '192.168.2.10'
        debug = False
        
        self.root= KpixDaq.DesyTrackerRoot(pollEn=False, ip=ip, debug=debug) 
        print ('Reading all')
        self.root.ReadAll()
        self.root.waitOnUpdate()
    
    def DoInitialise(self):        
        print ('DoInitialise')
        #print 'key_a(init) = ', self.GetInitItem("key_a")
        # Print the version info
        self.root.DesyTracker.AxiVersion.printStatus()

    def DoConfigure(self):        
        print ('DoConfigure')
        #print 'key_b(conf) = ', self.GetConfigItem("key_b")

    def DoStartRun(self):
        print ('DoStartRun')
        self.is_running = 1
        
    def DoStopRun(self):        
        print ('DoStopRun')
        self.is_running = 0

    def DoReset(self):        
        print ('DoReset')
        self.is_running = 0

    def RunLoop(self):
        print ("Start of RunLoop in ExamplePyProducer")
        trigger_n = 0;
        while(self.is_running):
            ev = pyeudaq.Event("RawEvent", "sub_name")
            ev.SetTriggerN(trigger_n)
            #block = bytes(r'raw_data_string')
            #ev.AddBlock(0, block)
            #print ev
            # Mengqing:
            datastr = 'raw_data_string'
            block = bytes(datastr, 'utf-8')
            ev.AddBlock(0, block)
            print(ev)
            
            self.SendEvent(ev)
            trigger_n += 1
            time.sleep(1)
        print ("End of RunLoop in ExamplePyProducer")

if __name__ == "__main__":
    myproducer = ExamplePyProducer("myproducer", "tcp://localhost:44000")
    print ("connecting to runcontrol in localhost:44000", )
    myproducer.Connect()
    time.sleep(2)
    while(myproducer.IsConnected()):
        time.sleep(1)
    myproducer.root.__exit__(None,None,None) # to manually terminate the kpix
    
