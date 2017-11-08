#! /usr/bin/env python
# load binary lib/pyeudaq.so
import pyeudaq
import time

class ExamplePyProducer(pyeudaq.Producer):
    is_running = 0
    def __init__(self, name, runctrl):
        pyeudaq.Producer.__init__(self, 'PyProducer', name, runctrl)
        print 'New instance of ExamplePyProducer'

    def DoInitialise(self):        
        print 'DoInitialise'
        print 'key_a(init) = ', self.GetInitItem("key_a")

    def DoConfigure(self):        
        print 'DoConfigure'
        print 'key_b(conf) = ', self.GetConfigItem("key_b")

    def DoStartRun(self):
        print 'DoStartRun'
        self.is_running = 1
        
    def DoStopRun(self):        
        print 'DoStopRun'
        self.is_running = 0

    def DoReset(self):        
        print 'DoReset'

    def RunLoop(self):
        print "Start of RunLoop in ExamplePyProducer"
        trigger_n = 0;
        while(self.is_running):
            ev = pyeudaq.Event("RawEvent", "sub_name")
            ev.SetTriggerN(trigger_n)
            print ev
            self.SendEvent(ev)
            trigger_n += 1
            time.sleep(1)
        print "End of RunLoop in ExamplePyProducer"

if __name__ == "__main__":
    myproducer = ExamplePyProducer("myproducer", "tcp://localhost:44000")
    print "connecting to runcontrol in localhost:44000", 
    myproducer.Connect()
    time.sleep(2)
    while(myproducer.IsConnected()):
        time.sleep(1)
