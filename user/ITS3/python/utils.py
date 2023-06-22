import pyeudaq
import sys
import logging

def exception_handler(method):
    def inner(*args, **kwargs):
        try:
            return method(*args, **kwargs)
        except Exception as e:
            pyeudaq.EUDAQ_ERROR(f"{method.__name__}: {e}")
            raise e
    return inner

class LoggingToEUDAQ:
    def __init__(self, eudaq_log=None):
        self.eudaq_log = eudaq_log
    
    def write(self, msg):
        if self.eudaq_log is not None: # user specified
            self.eudaq_log(msg.strip())
        elif msg.startswith("DEBUG"): # redirect logging to corresponding levels
            pyeudaq.EUDAQ_DEBUG(msg[6:].strip())
        elif msg.startswith("INFO"):
            pyeudaq.EUDAQ_INFO(msg[5:].strip())
        elif msg.startswith("WARNING"):
            pyeudaq.EUDAQ_WARN(msg[8:].strip())
        elif msg.startswith("ERROR"):
            pyeudaq.EUDAQ_ERROR(msg[6:].strip())
        elif msg.startswith("CRITICAL"):
            pyeudaq.EUDAQ_ERROR(msg[9:].strip())
        else:
            pyeudaq.EUDAQ_DEBUG(msg.strip())

    def flush(self):
        pass

def logging_to_eudaq(level=logging.DEBUG):
    logging.basicConfig(level=level,stream=LoggingToEUDAQ(),format='%(levelname)s: %(message)s')

# keepin this for future reference TODO implement or remove
# class StreamToEUDAQ:
#     def __init__(self, eudaq_log=pyeudaq.EUDAQ_DEBUG):
#         self.buffer = ''
#         self.eudaq_log = eudaq_log

#     def write(self, buf):
#         self.buffer += buf
#         lines = self.buffer.splitlines(True)
#         for line in lines[0:-1]:
#             self.eudaq_log(line)
#         if lines[-1][:-2] == '\n':
#             self.eudaq_log(lines[-1])
#         else:
#             self.buffer = lines[-1]

#     def flush(self):
#         if self.buffer:
#             self.eudaq_log(self.buffer)
#             self.buffer = ''

# def redirect_stdout_to_eudaq():
#     sys.stdout = StreamToEUDAQ()

