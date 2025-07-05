#! /usr/bin/env python3

import pyeudaq
from pyeudaq import EUDAQ_INFO, EUDAQ_ERROR
import time
from datetime import datetime
import numpy as np
import socket
import threading

def exception_handler(method):
    def inner(*args, **kwargs):
        try:
            return method(*args, **kwargs)
        except Exception as e:
            EUDAQ_ERROR(str(e))
            raise e
    return inner

class SourcemeterPyProducer(pyeudaq.Producer):
    def __init__(self, name, runctrl):
        pyeudaq.Producer.__init__(self, name, runctrl)
        self.name = name
        self.ip = None
        self.is_running = 0
        self.keithley = None
        self.OUTPUT = None
        self.lock=threading.Lock()
        EUDAQ_INFO('New instance of SourcemeterPyProducer')


    @exception_handler
    def DoInitialise(self):

        # restore default values, then parse the ini file
        self.do_reset = False
        self.source_voltage_range = None
        self.compliance_current = 20e-6
        self.biaslog = None

        iniList=self.GetInitConfiguration().as_dict()

        if 'IPaddress' not in iniList:
            raise ValueError('need to specify sourcemeter IPaddress in ini file')
        self.ip = iniList['IPaddress']
        EUDAQ_INFO(f'Keithley IPaddress = {self.ip}')
        if 'compliance' in iniList:
            self.compliance_current = float(iniList['compliance'])
        if 'voltageRange' in iniList:
            self.source_voltage_range = iniList['voltageRange']
        if 'resetSourcemeter' in iniList:
            self.do_reset = iniList['resetSourcemeter'].lower() in ("yes", "true", "t", "1")
        if 'biasLog' in iniList:
           self.biaslog = iniList['biasLog']
           self.OUTPUT = open(iniList['biasLog'], "a")

        # communicate with the sourcemeter
        self.keithley = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.keithley.connect((self.ip, 5025))
        self.keithley.sendall('*IDN?\n'.encode())
        time.sleep(0.01)

        # reset the device - think before you do this, this will also set output off!
        if self.do_reset:
            self.keithley.sendall('*RST\n'.encode())
            EUDAQ_INFO(f'Keithley sourcemeter was reset.')
            time.sleep(0.01)
        if self.source_voltage_range is not None:
            self.keithley.sendall((f'SOUR:VOLT:RANG {self.source_voltage_range}\n').encode())
            time.sleep(0.01)
            print((self.keithley.recv(1024)).decode())
            EUDAQ_INFO(f'source voltage range set to = {self.source_voltage_range}')
        else:
            self.keithley.sendall((f'SOUR:VOLT:RANG:AUTO ON\n').encode())
            time.sleep(0.01)
            print((self.keithley.recv(1024)).decode())
            EUDAQ_INFO(f'source voltage range set to AUTO')
        self.keithley.sendall((f':SOUR:VOLT:ILIM {self.compliance_current}\n').encode())
        EUDAQ_INFO(f'compliance set to = {self.compliance_current}')
        time.sleep(0.01)
        self.keithley.sendall('SOUR:FUNC VOLT\n'.encode())
        time.sleep(0.01)
        self.keithley.sendall('SENS:FUNC "CURR"\n'.encode())
        time.sleep(0.01)
        self.keithley.sendall('SENS:CURR:RANG:AUTO ON\n'.encode())
        time.sleep(0.01)
        self.keithley.sendall(':OUTP ON\n'.encode())
        time.sleep(0.01)

    @exception_handler
    def DoConfigure(self):
        EUDAQ_INFO('DoConfigure')
        confList = self.GetConfiguration().as_dict()
        if 'bias' not in confList:
            raise ValueError('need to specify bias in conf file')
        self.vtarget = int(confList['bias'])

        nsteps = 10
        steppause=2.0
        precision=0.01 # for comparing current and target voltage
        self.update_every = 15.0

        if 'update_every' in confList:
            self.update_every = float(confList['update_every'])
        if 'nsteps' in confList:
            nsteps = int(confList['nsteps'])
        if 'steppause' in confList:
            steppause = int(confList['steppause'])
        if 'precision' in confList:
            precision = float(confList['precision'])

        # more fancy would be to have a polarity init parameter in the future
        if self.vtarget > 0:
            raise ValueError('Check polarity. Please do not kill the sensor.')

        else:
            # get current voltage and ramp to there
            EUDAQ_INFO(f'Keithley set to ramp to {self.vtarget} V in {nsteps} steps')
            self.keithley.sendall(':MEAS:VOLT? "defbuffer1"\n'.encode())
            time.sleep(0.1)
            vmeas=float((self.keithley.recv(1024)).decode())
            if abs(vmeas-self.vtarget)<precision:
                EUDAQ_INFO(f'Target voltage {self.vtarget} and current voltage {vmeas} already agree within {precision}')
            else:
                stepsize=(self.vtarget - vmeas)/nsteps
                thesteps = np.arange(vmeas, self.vtarget+(stepsize*0.5), stepsize)
                for step in thesteps:
                    self.keithley.sendall((f'SOUR:VOLT {step}\n').encode())
                    time.sleep(steppause)
                    self.keithley.sendall(':MEAS:VOLT? "defbuffer1"\n'.encode())
                    volt=(float((self.keithley.recv(1024)).decode()))
                    self.keithley.sendall(':MEAS:CURR? "defbuffer2"\n'.encode())
                    curr=(float((self.keithley.recv(1024)).decode()))
                    timenow=datetime.now().strftime("%Y-%m-%d %I:%M:%S")
                    logline=f'{volt}V {curr*1e6}uA {timenow}'
                    EUDAQ_INFO(logline)
                    if self.biaslog is not None:
                        self.OUTPUT.write(f'{logline}\n')



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
        self.keithley.close()
        if self.biaslog is not None:
            self.OUTPUT.close()


    @exception_handler
    def DoTerminate(self):
        EUDAQ_INFO('DoTerminate')
        self.is_running = 0
        self.keithley.close()
        if self.biaslog is not None:
            self.OUTPUT.close()


    @exception_handler
    def RunLoop(self):
        EUDAQ_INFO("Start of RunLoop in SourcemeterPyProducer")
        trigger_n = 0;
        starttime=datetime.now()
        while(self.is_running):
            ev = pyeudaq.Event("RawEvent", "sub_name")
            ev.SetTriggerN(trigger_n)
            if (datetime.now() - starttime).total_seconds()>self.update_every:
                self.keithley.sendall(':MEAS:VOLT? "defbuffer1"\n'.encode())
                volt=(float((self.keithley.recv(1024)).decode()))
                self.keithley.sendall(':MEAS:CURR? "defbuffer2"\n'.encode())
                curr=(float((self.keithley.recv(1024)).decode()))
                timenow=datetime.now().strftime("%Y-%m-%d %I:%M:%S")
                logline=f'{volt}V {curr*1e6}uA {timenow}'
                EUDAQ_INFO(logline)
                if self.biaslog is not None:
                    self.OUTPUT.write(f'{logline}\n')
                    self.OUTPUT.flush()
                trigger_n += 1
                starttime=datetime.now()
            time.sleep(1)
        EUDAQ_INFO("End of RunLoop in SourcemeterPyProducer")


if __name__ == "__main__":
    import argparse
    parser=argparse.ArgumentParser(description='Sourcemeter python producer for Keithley 2450 or 2470',
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r', default='tcp://localhost:44000', help="where to find euRun")
    parser.add_argument('--name' ,'-n', default='KeithleyPyProducer', help="name of this producer instance")
    args=parser.parse_args()

    keiSMproducer = SourcemeterPyProducer(args.name,args.run_control)
    print (f'Connecting to runcontrol in {args.run_control}')
    keiSMproducer.Connect()
    time.sleep(2)
    while(keiSMproducer.IsConnected()):
        time.sleep(1)
