#! /usr/bin/env python3
# load binary lib/pyeudaq.so
import pyeudaq
from pyeudaq import EUDAQ_INFO, EUDAQ_ERROR
import time
from collections import deque

def exception_handler(method):
    def inner(*args, **kwargs):
        try:
            return method(*args, **kwargs)
        except Exception as e:
            EUDAQ_ERROR(str(e))
            raise e
    return inner

class ExamplePyDataCollector(pyeudaq.DataCollector):
    def __init__(self, name, runctrl):
        pyeudaq.DataCollector.__init__(self, 'PyDataCollector', name, runctrl)
        EUDAQ_INFO('New instance of ExamplePyDataCollector')
        self.dict_con_ev = {}

    @exception_handler
    def DoInitialise(self):
        EUDAQ_INFO('DoInitialise')
        # print ('key_a(init) = ', self.GetInitItem("key_a"))

    @exception_handler
    def DoConfigure(self):
        EUDAQ_INFO('DoConfigure')
        # print ('key_b(conf) = ', self.GetConfigItem("key_b"))

    @exception_handler
    def DoStartRun(self):
        EUDAQ_INFO('DoStartRun')
        self.dict_con_ev = {}

    @exception_handler        
    def DoReset(self):
        EUDAQ_INFO('DoReset')

    @exception_handler
    def DoConnect(self, con):
        t = con.GetType()
        n = con.GetName()
        r = con.GetRemote()
        EUDAQ_INFO("new producer connection: ", t, n, "from ", r)
        self.dict_con_ev[con] = deque()

    @exception_handler
    def DoDisconnect(self, con):
        t = con.GetType()
        n = con.GetName()
        r = con.GetRemote()
        EUDAQ_INFO("delete producer connection: ", t, n, "from ", r)
        del self.dict_con_ev[con]

    @exception_handler
    def DoReceive(self, con, ev):
        
        self.dict_con_ev[con].append(ev)
        
        for con, ev_queue in self.dict_con_ev.items():
            #print ( con, len(ev_queue) )
            if(len(ev_queue)==0):
                return

        sync_ev = pyeudaq.Event('RawEvent', 'mysyncdata')
        for con, ev_queue in self.dict_con_ev.items():
            sub_ev = ev_queue.popleft()
            sync_ev.AddSubEvent(sub_ev)
            
        self.WriteEvent(sync_ev)

        
if __name__ == "__main__":
    mydc = ExamplePyDataCollector("mydc", "tcp://localhost:44000")
    print ("connecting to runcontrol in localhost:44000", )
    mydc.Connect()
    time.sleep(2)
    while(mydc.IsConnected()):
        time.sleep(1)
