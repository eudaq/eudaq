#! /usr/bin/env python3
# load binary lib/pyeudaq.so
import pyeudaq
from pyeudaq import EUDAQ_INFO, EUDAQ_ERROR
import time

def exception_handler(method):
    def inner(*args, **kwargs):
        try:
            return method(*args, **kwargs)
        except Exception as e:
            EUDAQ_ERROR(str(e))
            raise e
    return inner

class ExamplePyProducer(pyeudaq.Producer):
    def __init__(self, name, runctrl):
        pyeudaq.Producer.__init__(self, name, runctrl)
        self.is_running = 0
        EUDAQ_INFO('New instance of ExamplePyProducer')

    @exception_handler
    def DoInitialise(self):        
        EUDAQ_INFO('DoInitialise')
        #print 'key_a(init) = ', self.GetInitItem("key_a")

    @exception_handler
    def DoConfigure(self):        
        EUDAQ_INFO('DoConfigure')
        #print 'key_b(conf) = ', self.GetConfigItem("key_b")

    @exception_handler
    def DoStartRun(self):
        EUDAQ_INFO('DoStartRun')
        self.is_running = 1
        
    @exception_handler
    def DoStopRun(self):        
        EUDAQ_INFO('DoStopRun')
        self.is_running = 0

    @exception_handler
    def DoReset(self):        
        EUDAQ_INFO('DoReset')
        self.is_running = 0

    @exception_handler
    def RunLoop(self):
        EUDAQ_INFO("Start of RunLoop in ExamplePyProducer")
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
        EUDAQ_INFO("End of RunLoop in ExamplePyProducer")

if __name__ == "__main__":
    myproducer = ExamplePyProducer("myproducer", "tcp://localhost:44000")
    print ("connecting to runcontrol in localhost:44000", )
    myproducer.Connect()
    time.sleep(2)
    while(myproducer.IsConnected()):
        time.sleep(1)
