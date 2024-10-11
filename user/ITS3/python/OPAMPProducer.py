#!/usr/bin/env python3

import pyeudaq
from APTSProducer import APTSProducer
from pyeudaq import EUDAQ_DEBUG, EUDAQ_ERROR
from mlr1daqboard.opamp_scope import OPAMPscope
from utils import exception_handler, logging_to_eudaq
from datetime import datetime
from time import sleep
import mlr1daqboard
import pickle
import socket
import struct
import subprocess

class ProducerComunication:
    # Class for the communication between the producers
    # The communication is based on the socket communication
    # One can initialize the class with the default values
    # or with the custom values.
    def __init__(self, socket_host_adress = "10.0.0.11", socket_port=15237, server_timeout=0.1, client_id=0, client_max_trial=1000, client_trial_interval=0.0001):
        self.socket_host_adress = socket_host_adress       # 127.0.0.1 (localhost)
        self.socket_port = socket_port                     # 15237 I(1), T(5), S(2), 3, 7(Luck)
        self.server_timeout = server_timeout               # 0.1s for 10Hz.
        self.client_id = client_id                         # 0, 1, 2.. in case of multiple client
        self.client_max_trial = client_max_trial           # 1000
        self.client_trial_interval = client_trial_interval # 0.0001 for 0.1 ms (10,000 Hz) fast enough
    
    def SendAndCheckAnswer(self, data_for_sending = "CONNECTION_TEST"):
        # SendAndCheckAnswer is a method for sending the data and checking the answer
        # 1) Send the data
        self.Send(data_for_sending)
        # 2) Wait for the answer
        answer = self.Receive()
        if answer == "OK":
            EUDAQ_DEBUG("Answer from receiver: OK")
            return True
        else:
            EUDAQ_ERROR("Failed to receive answer")
            raise RuntimeError("Failed to receive answer")
    
    def ReceiveAndAnswer(self):
        # ReceiveAndAnswer is a method for receiving the data and sending the answer
        # 1) Receive the data
        data = self.Receive()
        # 2) Send the answer
        if data is None:
            return False
        else:
            self.Send("OK")
            return data
    
    def Send(self, data_for_sending = "CONNECTION_TEST"):
        # Send is a method for sending the data
        EUDAQ_DEBUG(f" ProducerComunication: Send")
        # check if data_for_sending is byte or string
        isBytes = False
        try:
            data_for_sending = data_for_sending.decode()
            isBytes = True
        except (UnicodeDecodeError, AttributeError):
            pass
        if isBytes is False:
            data_for_sending = pickle.dumps(data_for_sending)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as socket_for_comm:
            socket_for_comm.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # allow reuse of the socket
            socket_for_comm.bind((self.socket_host_adress, self.socket_port))     # bind to the socket_port
            socket_for_comm.settimeout(self.server_timeout)                       # 0.1 ms: 10 Hz, worst case
            try:
                EUDAQ_DEBUG(" waiting for the client connection ")
                socket_for_comm.listen()                                          # start listening for connections
                conn, addr = socket_for_comm.accept()
                with conn:
                    data = conn.recv(1024)
                    EUDAQ_DEBUG(f" Connected by client #{data}: {addr} ")
                    self.send_msg(conn, data_for_sending) # send_msg(): used for sending a big file
                    EUDAQ_DEBUG(f" data sent size:{len(data_for_sending)}")
            except socket.error as err:
                EUDAQ_DEBUG(f" No connection from client {err}")
                raise f"No client connection {err}"
        return True
    
    def Receive(self):
        # Receive is a method for receiving the data
        EUDAQ_DEBUG(" ProducerComunication: Receive ")
        n_trial = 0
        max_trial = self.client_max_trial
        data = None
        while True:
            # Open the socket and try to connect to server
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as socket_for_comm:
                try:
                    socket_for_comm.connect((self.socket_host_adress, self.socket_port)) # connect to the socket_port
                    socket_for_comm.sendall(f"{self.client_id}".encode())                # send the client number
                    data = self.recv_msg(socket_for_comm)                                # recv_msg(): used for taking a big file
                    EUDAQ_DEBUG(f" data taken length {len(data)}")
                    data = pickle.loads(data) # decode the data
                    EUDAQ_DEBUG(f" decoded data type: {type(data)} ")
                except socket.error as err:
                    EUDAQ_DEBUG(f" Can't connect to server {err}")
            n_trial += 1
            if data or n_trial > max_trial:
                break
            sleep(self.client_trial_interval) # wait for the next trial
        if not data:
            EUDAQ_DEBUG('No received data')
        return data
            
    def send_msg(self, sock, msg):
        # Prefix each message with a 4-byte length (network byte order)
        msg = struct.pack('>I', len(msg)) + msg
        sock.sendall(msg)

    def recv_msg(self, sock):
        # Read message length and unpack it into an integer
        raw_msglen = self.recvall(sock, 4)
        if not raw_msglen:
            return None
        msglen = struct.unpack('>I', raw_msglen)[0]
        # Read the message data
        return self.recvall(sock, msglen)

    def recvall(self, sock, n):
        # Helper function to recv n bytes or return None if EOF is hit
        data = bytearray()
        while len(data) < n:
            packet = sock.recv(n - len(data))
            if not packet:
                return None
            data.extend(packet)
        return data

class OPAMPProducer(APTSProducer):
    def __init__(self, name, runctrl):
        APTSProducer.__init__(self,name,runctrl)
        self.scope  = None
        self.comm = None
        self.is_server = None                  # True: server connected to scope, False: client waiting for server
        self.scope_address = None              # 10.0.0.11
        self.scope_brand = None                # WaveMaster, Infiniium
        self.socket_host_adress = None         # "127.0.0.1"
        self.socket_port = None                # 15237 I(1), T(5), S(2), 3, 7(Luck)
        self.server_timeout = None             # 0.1s for 10Hz.
        self.client_id = None                  # 0, 1, 2.. in case of multiple client
        self.client_max_trial = None           # 1000
        self.client_trial_interval = None      # 0.0001 for 0.1 ms (10,000 Hz) fast enough
        self.number_of_ch_to_save = None       # number of channels to be saved: from 1 to 4. (Default: 4)
        self.channels_to_send = None           # Channels to send (server): can be separated with , eg. 3,4 (optional)
        self.sequence_mode = None              # True if we want to use the scope sequence mode (automatically set if n_segment_scope_sequence > 1)
        self.n_point_per_waveform = None       # Number of points per waveform (automatically set based on the tdiv and ns_sample)
        self.header_length = 16                # Number of bytes in the header in seuqence mode (NOTE: only used in the sequence mode)
        self.trailer_length = 1                # Number of bytes in the trailer in seuqence mode (NOTE: only used in the sequence mode)

    @exception_handler
    def DoInitialise(self):
        APTSProducer.DoInitialise(self)
        conf=self.GetInitConfiguration().as_dict()
        scope_class_list = [s for s in OPAMPscope.__subclasses__()]
        self.is_server = (conf['is_server'].lower() in ['true','yes','1']) if 'is_server' in conf else False 
        self.scope_timeout = int(conf['timeout']) if 'timeout' in conf else 25
        chs=conf['channels'].split('&')  if 'channels' in conf else [1,2,3,4]  # in conf file we have something like trigger=1&2&3&4
        self.channels = [int(c) for c in chs]
        self.scope_brand = conf['scope_brand'] if 'scope_brand' in conf else None
        assert self.scope_brand in str([s.__name__ for s in OPAMPscope.__subclasses__()]), "Specify scope brand within " + str([s.__name__ for s in OPAMPscope.__subclasses__()])
        self.number_of_ch_to_save = int(conf['number_of_ch_to_save']) if 'number_of_ch_to_save' in conf else 4
        self.channels_to_send = [int(x) for x in conf['channels_to_send'].split(",")] if 'channels_to_send' in conf else []
        if self.is_server:
            self.scope_address = conf['scope_address'] if 'scope_address' in conf else None
            # instantiate oscilloscope subclass of OPAMPscope
            for scope in scope_class_list:
                if scope.__name__ == self.scope_brand:
                    self.scope = scope(address=self.scope_address, timeout_sec=self.scope_timeout, active_channels=self.channels)
            assert self.scope is not None, f"Cannot connect to {self.scope_brand}"
            
        if len(self.channels_to_send) > 1: # communication mode
            EUDAQ_DEBUG(" Communication mode ")
            self.socket_host_adress = conf['socket_host_adress'] 
            self.socket_port = int(conf['socket_port'])
            self.server_timeout = int(conf['server_timeout'])
            self.client_id = int(conf['client_id'])
            self.client_max_trial = int(conf['client_max_trial'])
            self.client_trial_interval = float(conf['client_trial_interval'])
            self.comm = ProducerComunication(socket_host_adress = self.socket_host_adress, socket_port=self.socket_port, server_timeout=self.server_timeout, client_id=self.client_id, client_max_trial=self.client_max_trial, client_trial_interval=self.client_trial_interval)
            if self.is_server:
                self.comm.SendAndCheckAnswer() 
            else:
                self.comm.ReceiveAndAnswer()
    
    @exception_handler
    def DoConfigure(self):        
        APTSProducer.DoConfigure(self)
        conf=self.GetConfiguration().as_dict()
        if self.is_server:
            # channel by channel configuration
            channel_terminations = []
            vdivs = []
            channel_offsets = []
            trigger_on_channels = []
            baselines = []
            trigger=conf['trigger'].split('&')  # in conf file we have something like trigger=C1&C2&C3&C4 (written channels are the ones that compare the scope trigger, with an OR logic)
            for ich in range(1,5):
                channel_terminations.append(str(conf[f'channel_termination_{ich}'])) # 50 Ohm, 1 MOhm
                vdivs.append(float(conf[f'vdiv{ich}']))
                channel_offsets.append(float(conf[f'channel_offset_{ich}'])) if f'channel_offset_{ich}' in conf else None
                trigger_on_channels.append(True if (f"C{ich}" in trigger) else False)
                baselines.append(float(conf[f'baseline_{ich}'])) if f'baseline_{ich}' in conf else None
            # global configuration
            tdiv=float(conf['tdiv']) # Actual time windows is tdiv * self.scope.TIME_DIVISION (10)
            scope_sampling_period_s=float(conf['scope_sampling_period_s'])
            window_delay_s=float(conf['window_delay_s'])
            trg_delay_s=float(conf['trg_delay_s'])
            offset_search=(conf['offset_search'].lower() in ['true','yes','1']) if 'offset_search' in conf else False
            trigger_slope=conf['trigger_slope'] # RISing, FALLing for Infiniium, H, L for WaveMaster
            relative_trigger_level_volt=float(conf['relative_trigger_level_volt'])
            auxiliary_output=conf['auxiliary_output'] # TriggerEnabled, TriggerOut

            # Check the configuration
            if offset_search is False:
                assert all([x is not None for x in channel_offsets]), "If offset_search is False, channel_offsets should be specified"
            if self.scope_brand == 'Infiniium':
                assert all([x is not None for x in baselines]), "If scope brand is Infiniium, baselines should be specified"

            # Apply configuration to scope
            self.scope.clear()
            self.scope.configure(channels_termination=tuple(channel_terminations), vdiv=tuple(vdivs), tdiv=tdiv, sampling_period_s=scope_sampling_period_s, window_delay_s=window_delay_s, trg_delay_s=trg_delay_s)
            self.scope.set_offset(search=offset_search, channels_offset=tuple(channel_offsets))
            self.scope.set_trigger(trigger_on_channel=tuple(trigger_on_channels), baseline=tuple(baselines), slope=trigger_slope, relative_trigger_level_volt=relative_trigger_level_volt)
            self.scope.set_auxiliary_output(mode=auxiliary_output)

            # Setting sequence mode
            self.n_point_per_waveform = self.scope.TIME_DIVISION*float(conf['tdiv'])/float(conf['scope_sampling_period_s'])
            if 'n_segment_scope_sequence' in conf:
                EUDAQ_DEBUG(" Sequence mode ")
                self.sequence_mode = True 
                self.n_segment_scope_sequence = int(conf['n_segment_scope_sequence'])
                sequence_timeout_mode=conf['sequence_timeout_mode'] if 'sequence_timeout_mode' in conf else False
                sequence_timeout_s=conf['sequence_timeout_s'] if 'sequence_timeout_s' in conf else None
                self.scope.set_sequence_mode(nseg=self.n_segment_scope_sequence, timeout_enable=sequence_timeout_mode, sequence_timeout=sequence_timeout_s)
                if self.is_server == False:
                    raise Exception(" Sequence mode not available in client mode ")
    
    @exception_handler
    def DoStartRun(self):
        self.idev=0
        self.isev=0
        self.send_status_event(datetime.now(),bore=True)
        self.isev+=1
        while self.daq.read_data(1<<16,timeout=100) is not None:
            pyeudaq.EUDAQ_WARN('Dirty buffer, purging...') # this should not happen
        if self.is_server: self.scope.arm_trigger() # prepare the data acquisition, trigger ready.
        self.daq.write_register(3,0xAC,1) # request next data / arm trigger
        self.is_running=True
        if self.sequence_mode is True: # Set the data buffer for the sequence mode
            self.dataADC = []
            self.tsdata = []
    
    @exception_handler
    def DoTerminate(self):
        self.scope.clear()
        del self.scope
    
    @exception_handler
    def RunLoop(self):
        # status events: roughly every 10s (time checked at least every 1000 events or at receive timeout)
        timelast=datetime.now()
        idevlast=0
        export_axis_info_done = False
        while self.is_running:
            if self.read_and_send_event() and not export_axis_info_done:
                # Export axis info only once, right after the first successful event read
                self.send_status_event(timelast, exp_var=True)
                export_axis_info_done = True
            # Check status at regular intervals or every 1000 events
            if (self.idev - idevlast) % 1000 == 0 or (datetime.now() - timelast).total_seconds() >= 10:
                timelast = datetime.now()
                idevlast = self.idev
                self.send_status_event(timelast)
        timelast=datetime.now()
        self.send_status_event(timelast)
        sleep(1)
        while self.read_and_send_event(): # try to get anything remaining TODO: timeout?
            EUDAQ_DEBUG(" read remaining events ")
        timelast=datetime.now()
        self.send_status_event(timelast,eore=True)

    def read_and_send_event(self):
        if self.sequence_mode == True:
            return self.read_and_send_event_sequence()

        EUDAQ_DEBUG(" read DAQ board event ")
        data = self.daq.read_data(40960,timeout=100) # sufficent for up to 1k frames
        if data is None: 
            return False
        
        data = bytes(data)
        tsdata = bytes(self.daq.read_data(12,timeout=100))
        trg_ts = mlr1daqboard.decode_trigger_timestamp(tsdata)

        osc_data = None
        if self.is_server: # is_server
            EUDAQ_DEBUG(" read oscilloscope event ")
            osc_data = self.scope.readout() # [ch1,ch2,ch3,ch4]
            # in case of no input from the scope, return False
            if all(d is None for d in osc_data):
                raise ValueError(" no oscilloscope data!! ")

        if len(self.channels_to_send) > 1: # communication mode
            if self.is_server: # is_server
                sendList = []
                for ich in self.channels_to_send:
                    sendList.append(osc_data[ich])
                data_to_send = pickle.dumps(sendList)
                EUDAQ_DEBUG(f" data packed size: {len(data_to_send)}")

                # Open the socket and accept a connection from client
                if self.comm.SendAndCheckAnswer(data_to_send) is not True:
                    raise RuntimeError("failed to send the data")
            else:
                osc_data = self.comm.ReceiveAndAnswer()
                if osc_data is None:
                    raise RuntimeError("failed to receive data")

        EUDAQ_DEBUG(f" type: {type(osc_data[0])} ")
        EUDAQ_DEBUG(" save event OPAMPProducer.py ")

        ev = pyeudaq.Event("RawEvent", self.name)
        ev.SetTriggerN(self.idev)
        ev.SetTimestamp(begin=trg_ts, end=trg_ts)
        ev.SetDeviceN(self.plane)
        ev.AddBlock(0,data)
        ev.AddBlock(1,tsdata)
        for ich in range(self.number_of_ch_to_save):
            ev.AddBlock(2+ich, osc_data[ich])
        self.SendEvent(ev)
        self.idev+=1
        
        if len(self.channels_to_send) > 1: # Wait for the handshake
            if self.is_server: # is_server
                self.comm.SendAndCheckAnswer() 
            else:
                self.comm.ReceiveAndAnswer()

        if self.is_running: 
            if self.is_server: self.scope.arm_trigger() # prepare the next data acquisition, trigger ready.
            self.daq.write_register(3,0xAC,1,log_level=9) # request next data / arm trigger
        return True
    
    def read_and_send_event_sequence(self):
        data = self.daq.read_data(40960,timeout=100)
        if data is not None: 
            self.tsdata.append(self.daq.read_data(12,timeout=100))
            self.dataADC.append(data)
        if 'STOP' not in str(self.scope.get_trigger_mode()):
            self.daq.write_register(3,0xAC,1,log_level=9)
            return False

        EUDAQ_DEBUG(" read oscilloscope event ")
        osc_data_sequence = self.scope.readout()
        # Check the total length of the data
        for ich in range(4):
            assert len(osc_data_sequence[ich]) == self.header_length+self.n_point_per_waveform*self.n_segment_scope_sequence+self.trailer_length, " read_and_send_event_sequence: osc_data_sequence has wrong length "
        
        for jev in range(self.n_segment_scope_sequence):
            trg_ts = mlr1daqboard.decode_trigger_timestamp(self.tsdata[jev])
            ev = pyeudaq.Event("RawEvent", self.name)
            ev.SetEventN(self.idev)
            ev.SetTriggerN(self.idev)
            ev.SetTimestamp(begin=trg_ts, end=trg_ts)
            ev.SetDeviceN(self.plane)
            ev.AddBlock(0,bytes(self.dataADC[jev]))
            ev.AddBlock(1,bytes(self.tsdata[jev]))
            for ich in range(self.number_of_ch_to_save):
                ev.AddBlock(2+ich, osc_data_sequence[ich][int(self.header_length+jev*self.n_point_per_waveform):int(self.header_length+(jev+1)*self.n_point_per_waveform)])
            self.SendEvent(ev)
            self.idev+=1
        if self.is_server: self.scope.arm_trigger() # prepare the next data acquisition, trigger ready.
        self.daq.write_register(3,0xAC,1,log_level=9) # request next data / arm trigger
        
        # reset the data buffer
        self.dataADC = []
        self.tsdata = []
        return True
    
    def send_status_event(self,time,bore=False,eore=False,exp_var=False):
        ev=pyeudaq.Event('RawEvent',self.name+'_status')
        ev.SetTag('Event','%d'%self.idev)
        ev.SetTag('Time',time.isoformat())
        ev.SetTag('Ia','%.2f mA'%self.daq.read_isenseA())
        ev.SetTag('Temperature','%.2f'%self.daq.read_temperature())
        if exp_var:
            dt,t0,dvs,v0s = self.scope.get_waveform_axis_variables()
            if dt is not None:
                ev.SetTag('Scope dt (ps)','%f'%(dt*1E12))
                ev.SetTag('Scope t0 (ns)','%f'%(t0*1E9))
                for ich in range(4): # Saving the scope settings
                    ev.SetTag(f'Scope CH{ich+1} dv (V)','%f'%dvs[ich])
                    ev.SetTag(f'Scope CH{ich+1} v0 (V)','%f'%v0s[ich])
        if bore:
            ev.SetBORE()
            git =subprocess.check_output(['git','rev-parse','HEAD']).strip()
            diff=subprocess.check_output(['git','diff'])
            ev.SetTag('EUDAQ_GIT'  ,git )
            ev.SetTag('EUDAQ_DIFF' ,diff)
            ev.SetTag('MLR1SW_GIT' ,mlr1daqboard.get_software_git())
            ev.SetTag('MLR1SW_DIFF',mlr1daqboard.get_software_diff())
            ev.SetTag('FW_VERSION' ,self.daq.read_fw_version())
            ev.SetTag('SEQUENCE_MODE', "True" if self.sequence_mode else "False")
        if eore:
            ev.SetEORE()

        self.SendEvent(ev)
        self.isev+=1

if __name__ == "__main__":
    import argparse
    parser=argparse.ArgumentParser(description='OPAMP EUDAQ2 Producer',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r',default='tcp://localhost:44000')
    parser.add_argument('--name' ,'-n',default='OPAMP_XXX')
    args=parser.parse_args()

    logging_to_eudaq()

    myproducer=OPAMPProducer(args.name,args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        sleep(1)
