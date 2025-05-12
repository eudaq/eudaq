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
    def __init__(self, name, runctrl, makebuffers):
        pyeudaq.Producer.__init__(self, name, runctrl)
        self.name = name
        self.ip=None
        self.is_running = 0
        self.keithley=None
        self.makeBuffers=makebuffers
        self.lock=threading.Lock()
        EUDAQ_INFO('New instance of SourcemeterPyProducer')


    @exception_handler
    def DoInitialise(self):

        # parse the ini file
        iniList=self.GetInitConfiguration().as_dict()

        if 'IPaddress' not in iniList:
            raise ValueError('need to specify sourcemeter IPaddress in ini file')
        self.ip = iniList['IPaddress']
        EUDAQ_INFO(f'Keithley IPaddress = {self.ip}')

        self.compliance_current = 20e-6
        if 'compliance' in iniList:
            self.compliance_current = float(iniList['compliance'])
        EUDAQ_INFO(f'compliance will be set to = {self.compliance_current}')

        # could also make these ini parameters in the future
        self.source_voltage_range = 200

        doReset = False
        if 'resetSourcemeter' in iniList:
            doReset = iniList['resetSourcemeter'].lower() in ("yes", "true", "t", "1")

        # communicate with the sourcemeter
        self.keithley = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.keithley.connect((self.ip, 5025))
        self.keithley.sendall('*IDN?\n'.encode())
        time.sleep(0.01)

        # reset the device - think before you do this, this will also set output off!
        if doReset:
            self.keithley.sendall('*RST\n'.encode())
            self.makeBuffers=True
            EUDAQ_INFO(f'Keithley sourcemeter was reset.')
            time.sleep(0.01)

        self.keithley.sendall((f'SOUR:VOLT:RANG {self.source_voltage_range}\n').encode())
        time.sleep(0.01)
        print((self.keithley.recv(1024)).decode())

        self.keithley.sendall((f':SOUR:VOLT:ILIM {self.compliance_current}\n').encode())
        time.sleep(0.01)
        self.keithley.sendall('SOUR:FUNC VOLT\n'.encode())
        time.sleep(0.01)
        self.keithley.sendall('SENS:FUNC "CURR"\n'.encode())
        time.sleep(0.01)
        self.keithley.sendall('SENS:CURR:RANG:AUTO ON\n'.encode())
        time.sleep(0.01)
        self.keithley.sendall(':OUTP ON\n'.encode())
        time.sleep(0.01)
        # create the buffers if requested (just the first time after starting)
        if self.makeBuffers:
            EUDAQ_INFO(f'Keithley sourcemeter creating buffers for voltage and current.')
            self.keithley.sendall('TRACe:MAKE "voltMeas", 15\n'.encode())
            time.sleep(0.01)
            self.keithley.sendall('TRACe:MAKE "currMeas", 15\n'.encode())
            time.sleep(0.01)
            # use continuous fill mode - with circular buffer, I don't have to worry about overflow
            self.keithley.sendall('TRACe:FILL:MODE CONT, "voltMeas"\n'.encode())
            time.sleep(0.01)
            self.keithley.sendall('TRACe:FILL:MODE CONT, "currMeas"\n'.encode())
            time.sleep(0.01)
            self.makeBuffers=False


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
            EUDAQ_INFO(f'Keithley set to ramp to {self.vtarget} V in {nsteps} steps')
            #could also be parameters: steps, pause-in-seconds
            # get current voltage and ramp to there
            self.keithley.sendall(':MEAS:VOLT? "voltMeas"\n'.encode())
            time.sleep(0.1)
            vmeas=float((self.keithley.recv(1024)).decode())
            if abs(vmeas-self.vtarget)<precision:
                EUDAQ_INFO(f'Target voltage {self.vtarget} and current voltage {vmeas} already agree within {precision}')
            else:
                stepsize=(self.vtarget - vmeas)/nsteps
                thesteps = np.arange(vmeas, self.vtarget+(stepsize*0.5), stepsize)
                print(thesteps)
                for step in thesteps:
                    self.keithley.sendall((f'SOUR:VOLT {step}\n').encode())
                    time.sleep(steppause)
                    self.keithley.sendall(':MEAS:VOLT? "voltMeas"\n'.encode())
                    volt=(float((self.keithley.recv(1024)).decode()))
                    self.keithley.sendall(':MEAS:CURR? "currMeas"\n'.encode())
                    curr=(float((self.keithley.recv(1024)).decode()))
                    self.keithley.sendall(':MEAS? "defbuffer1", SEC\n'.encode())
                    t=(float((self.keithley.recv(1024)).decode()))
                    logline=f'{volt}V {curr*1e6}uA {t}s'
                    #print(logline)
                EUDAQ_INFO(logline)


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
        # close the socket connection
        self.keithley.close()


    @exception_handler
    def RunLoop(self):
        EUDAQ_INFO("Start of RunLoop in SourcemeterPyProducer")
        trigger_n = 0;
        starttime=datetime.now()
        while(self.is_running):
            ev = pyeudaq.Event("RawEvent", "sub_name")
            ev.SetTriggerN(trigger_n)
            if (datetime.now() - starttime).total_seconds()>self.update_every:
                self.keithley.sendall(':MEAS:VOLT? "voltMeas"\n'.encode())
                volt=(float((self.keithley.recv(1024)).decode()))
                self.keithley.sendall(':MEAS:CURR? "currMeas"\n'.encode())
                curr=(float((self.keithley.recv(1024)).decode()))
                self.keithley.sendall(':MEAS? "defbuffer1", SEC\n'.encode())
                t=(float((self.keithley.recv(1024)).decode()))
                logline=f'{volt}V {curr*1e6}uA {t}s'
                EUDAQ_INFO(logline)
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
    parser.add_argument('--create-buffers', action="store_true", help="force creation of measurement buffers, e.g. after power-cycle")
    args=parser.parse_args()

    if args.create_buffers:
        keiSMproducer = SourcemeterPyProducer(args.name,args.run_control, makebuffers=True)
    else:
        keiSMproducer = SourcemeterPyProducer(args.name,args.run_control, makebuffers=False)
    print (f"connecting to runcontrol in {args.run_control}")
    keiSMproducer.Connect()
    time.sleep(2)
    while(keiSMproducer.IsConnected()):
        time.sleep(1)



