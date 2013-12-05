import sys
from ctypes import cdll, create_string_buffer, byref, c_uint

libpath = '../lib/libPyEUDAQ'
libext = '.so'
if sys.platform.startswith('darwin'):
    # OSX-specific library extension
    libext = '.dylib'
lib = cdll.LoadLibrary(libpath+libext)

class PyRunControl(object):
    def __init__(self,addr):
        self.obj = lib.PyRunControl_new(create_string_buffer(addr))

    def GetStatus(self):
        lib.PyRunControl_GetStatus(self.obj)

    def StartRun(self):
        lib.PyRunControl_StartRun(self.obj)

    def StopRun(self):
        lib.PyRunControl_StopRun(self.obj)

    def Configure(self,cfg):
        lib.PyRunControl_Configure(self.obj, create_string_buffer(cfg))

    def NumConnections(self):
        return lib.PyRunControl_NumConnections(self.obj)


class PyProducer(object):
    def __init__(self,rcaddr, addr):
        self.obj = lib.PyProducer_new(create_string_buffer(rcaddr), 
                                      create_string_buffer(addr))
    def Event(self,data):
        lib.PyProducer_Event(self.obj,create_string_buffer(data))

    def RandomEvent(self,size):
        lib.PyProducer_RandomEvent(self.obj,c_uint(size))

class PyDataCollector(object):
    def __init__(self,rcaddr, addr):
        self.obj = lib.PyDataCollector_new(create_string_buffer(rcaddr), 
                                           create_string_buffer(addr))
