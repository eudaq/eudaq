import sys
from ctypes import cdll, create_string_buffer, byref, c_uint, c_void_p, c_char_p, c_size_t, c_uint64, POINTER
import numpy

libpath = '../lib/libPyEUDAQ'
libext = '.so'
if sys.platform.startswith('darwin'):
    # OSX-specific library extension
    libext = '.dylib'
elif sys.platform.startswith('win32') or sys.platform.startswith('cygwin'):
    # Windows-specific library extension
    libext = '.dll'
lib = cdll.LoadLibrary(libpath+libext)

class PyRunControl(object):
    def __init__(self,addr = "tcp://44000"):
        lib.PyRunControl_new.restype = c_void_p # Needed
        self.obj = lib.PyRunControl_new(create_string_buffer(addr))
    def GetStatus(self):
        lib.PyRunControl_GetStatus(c_void_p(self.obj))
    def StartRun(self):
        lib.PyRunControl_StartRun(c_void_p(self.obj))
    def StopRun(self):
        lib.PyRunControl_StopRun(c_void_p(self.obj))
    def Configure(self,cfg):
        lib.PyRunControl_Configure(c_void_p(self.obj), create_string_buffer(cfg))
    @property 
    def NumConnections(self):
        return lib.PyRunControl_NumConnections(c_void_p(self.obj))
    @property 
    def RunNumber(self):
        return lib.PyRunControl_GetRunNumber(c_void_p(self.obj))
    @property 
    def AllOk(self):
        return lib.PyRunControl_AllOk(c_void_p(self.obj))

lib.PyProducer_SendEvent.argtypes = [c_void_p,POINTER(c_uint64), c_size_t]
class PyProducer(object):
    def __init__(self, name, rcaddr = "tcp://localhost:44000"):
        lib.PyProducer_new.restype = c_void_p # Needed
        self.obj = lib.PyProducer_new(create_string_buffer(name), 
                                      create_string_buffer(rcaddr))
    def SendEvent(self,data):
        data = data.astype(numpy.uint64)
        data_p = data.ctypes.data_as(POINTER(c_uint64))
        lib.PyProducer_SendEvent(c_void_p(self.obj),data_p,data.size)
    def GetConfigParameter(self, item):
        return c_char_p(lib.PyProducer_GetConfigParameter(c_void_p(self.obj),create_string_buffer(item))).value
    @property
    def Configuring(self):
        return lib.PyProducer_IsConfiguring(c_void_p(self.obj))
    @Configuring.setter
    def Configuring(self, value):
        if value:
            # try to set configured state, otherwise set error state
            if not lib.PyProducer_SetConfigured(c_void_p(self.obj)):
             lib.PyProducer_SetError(c_void_p(self.obj))
        else:
            # if we set configured to 'false' there has been an error
            lib.PyProducer_SetError(c_void_p(self.obj))

    @property
    def StartingRun(self):
        return lib.PyProducer_IsStartingRun(c_void_p(self.obj))
    @StartingRun.setter
    def StartingRun(self, value):
        if value:
            # try to set running state/send BORE, otherwise set error state
            if not lib.PyProducer_SendBORE(c_void_p(self.obj)):
             lib.PyProducer_SetError(c_void_p(self.obj))
        else:
            # if we set this property to 'false' there has been an error
            lib.PyProducer_SetError(c_void_p(self.obj))
    @property
    def StoppingRun(self):
        return lib.PyProducer_IsStoppingRun(c_void_p(self.obj))
    @StoppingRun.setter
    def StoppingRun(self, value):
        if value:
            # try to set stopped state/send EORE, otherwise set error state
            if not lib.PyProducer_SendEORE(c_void_p(self.obj)):
             lib.PyProducer_SetError(c_void_p(self.obj))
        else:
            # if we set this property to 'false' there has been an error
            lib.PyProducer_SetError(c_void_p(self.obj))
    @property
    def Terminating(self):
        return lib.PyProducer_IsTerminating(c_void_p(self.obj))
    @property
    def Error(self):
        return lib.PyProducer_IsError(c_void_p(self.obj))


class PyDataCollector(object):
    def __init__(self,name = "", rcaddr = "tcp://localhost:44000", listenaddr = "tcp://44001"):
        lib.PyDataCollector_new.restype = c_void_p # Needed
        self.obj = lib.PyDataCollector_new(create_string_buffer(name),
                                           create_string_buffer(rcaddr), 
                                           create_string_buffer(listenaddr))
