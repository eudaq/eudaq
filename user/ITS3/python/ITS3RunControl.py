#!/usr/bin/env python3
import pyeudaq
import threading
import urwid
import urwid_timed_progress
import asyncio
import re
from time import sleep
import argparse
import traceback
from utils import exception_handler

urwid_timed_progress.FancyProgressBar.get_text=lambda self: '%d/%d %s (%.1f%%)'%(self.current,self.done,self.units[0][0],self.current/self.done*100)

class ITS3TUI():
    PALETTE=[
      ('state-error'        ,'white,bold,blink'    ,'dark red'   ),
      ('state-conwait'      ,'dark red'            ,'black'      ),
      ('state-uninit'       ,'dark red'            ,'black'      ),
      ('state-unconf'       ,'dark red'            ,'black'      ),
      ('state-conf'         ,'yellow'              ,'black'      ),
      ('state-running'      ,'light green'         ,'black'      ),
      ('state-stopped'      ,'dark green'          ,'black'      ),
      ('bg'                 ,'light green'         ,'black'      ),
      ('streak'             ,'light green'         ,'black'      ),
      ('bt'                 ,'white'               ,'black'      ),
      ('header'             ,'light green'         ,'black'      ),
      ('footer'             ,'white'               ,'dark blue'  ),
      ('progress-normal'    ,'black'               ,'dark green' ),
      ('progress-complete'  ,'black'               ,'light green'),
      ('table-header'       ,'white,bold,underline','black'      ),
      ('div'                ,'dark green'          ,'black'      ),
    ]
    GLOBAL_STATES={
      'CONWAIT':('state-conwait'      ,'CONNECT WAIT'   ),
      'ERROR'  :('state-error'        ,' - ERROR - '    ),
      'UNINIT' :('state-uninit'       ,'UNINITIALISED'  ),
      'UNCONF' :('state-unconf'       ,'UNCONFIGURED'   ),
      'CONF'   :('state-conf'         ,'CONFIGURED'     ),
      'RUNNING':('state-running'      ,' >> RUNNING >> '),
      'STOPPED':('state-stopped'      ,'STOPPED'        ),
      'TERMINATED':('state-uninit'    ,'TERMINATED'     ),
    }
    CONNECTION_STATES={
      pyeudaq.Status.STATE_ERROR  :('state-error'  ,'--ERROR--'    ),
      pyeudaq.Status.STATE_UNINIT :('state-unini'  ,'UNINITIALISED'),
      pyeudaq.Status.STATE_UNCONF :('state-unconf' ,'UNCONFIGURED' ),
      pyeudaq.Status.STATE_CONF   :('state-conf'   ,'CONFIGURED'   ),
      pyeudaq.Status.STATE_STOPPED:('state-stopped','STOPPED'      ),
      pyeudaq.Status.STATE_RUNNING:('state-running','RUNNING'      ),
    }

    def __init__(self,rc):
        self.rc=rc
        self.statefont=urwid.font.HalfBlock7x7Font()
        self.tui=self.buildtui()
        self.aloop=asyncio.get_event_loop()

    def handle_keys(self,key):
        if key in ('Q'):
            if self.rc.halt:
                raise urwid.ExitMainLoop()
        elif key in ('T'):
            with self.rc.lock:
                self.rc.halt = True
                self.rc.running = False
        elif key in ('S'):
            with self.rc.lock:
                self.rc.running = False

    def run(self):
        self.loop=urwid.MainLoop(
          self.tui,
          self.PALETTE,
          event_loop=urwid.AsyncioEventLoop(loop=self.aloop),
          unhandled_input=self.handle_keys,
          handle_mouse=False
        )
        self.state.set_text(self.GLOBAL_STATES['CONWAIT'])
        self.loop.run()

    # TODO: can this become an async lambda?
    async def set_state_(self,state):
        self.state.set_text(state)
        self.loop.draw_screen()
    def set_state(self,state):
        asyncio.run_coroutine_threadsafe(self.set_state_(self.GLOBAL_STATES[state]),self.aloop).result()

    async def update_producer_(self,producer,cols):
        for a,b in zip(self.producers[producer],cols):
            a.set_text(b)
        self.loop.draw_screen()
    def update_producer(self,producer,state,ndev,nsev,info):
        cols=[producer,self.CONNECTION_STATES[state],'%8d'%ndev,'%8d'%nsev,info]
        asyncio.run_coroutine_threadsafe(self.update_producer_(producer,cols),self.aloop).result()

    async def update_collector_(self,collector,cols):
        for a,b in zip(self.collectors[collector],cols):
            a.set_text(b)
        self.loop.draw_screen()
    def update_collector(self,collector,state,ndev,nsev,info):
        if 'warning' in info.lower(): info = ('state-conf', info)
        cols=[collector,self.CONNECTION_STATES[state],'%8d'%ndev,'%8d'%nsev,info]
        asyncio.run_coroutine_threadsafe(self.update_collector_(collector,cols),self.aloop).result()

    async def update_logger_(self,logger,cols):
        for a,b in zip(self.collectors[logger],cols):
            a.set_text(b)
        self.loop.draw_screen()
    def update_logger(self,logger,state,info):
        cols=[logger,self.CONNECTION_STATES[state],'','',info]
        asyncio.run_coroutine_threadsafe(self.update_logger_(logger,cols),self.aloop).result()

    async def update_progress_run_(self,nev):
        if self.progress_run.current+nev>self.progress_run.done:
            self.progress_run.add_progress(nev,self.progress_run.current+nev)
        else:
            self.progress_run.add_progress(nev)
        self.loop.draw_screen()
    def update_progress_run(self,nev):
        asyncio.run_coroutine_threadsafe(self.update_progress_run_(nev),self.aloop).result()

    async def target_progress_run_(self,ntot):
        self.progress_run.done=ntot
        self.loop.draw_screen()
    def target_progress_run(self,ntot):
        asyncio.run_coroutine_threadsafe(self.target_progress_run_(ntot),self.aloop).result()

    async def reset_progress_run_(self):
        self.progress_run.reset()
        self.loop.draw_screen()
    def reset_progress_run(self):
        asyncio.run_coroutine_threadsafe(self.reset_progress_run_(),self.aloop).result()

    async def update_progress_tot_(self):
        self.progress_tot.add_progress(1)
        self.loop.draw_screen()
    def update_progress_tot(self):
        asyncio.run_coroutine_threadsafe(self.update_progress_tot_(),self.aloop).result()

    async def update_footer_(self,text):
        self.footer.set_text(text)
        self.loop.draw_screen()
    def update_footer(self,text):
        asyncio.run_coroutine_threadsafe(self.update_footer_(text),self.aloop).result()

    async def update_configs_list_(self,text):
        self.configs_list.set_text(text)
        self.loop.draw_screen()
    def update_configs_list(self,configs_list):
        text = [('table-header', configs_list[0]), ', ', ', '.join(configs_list[1:])]
        asyncio.run_coroutine_threadsafe(self.update_configs_list_(text),self.aloop).result()

    def buildtui(self):
        self.footer=urwid.Text(('footer','Hotkeys: [S]top run, [T]erminate, [Q]uit'))
        footer=urwid.Padding(self.footer)
        footer=urwid.AttrWrap(footer,'footer')
        header=urwid.Columns([
          ('pack',urwid.Text('ITS3 Run Control')),
          urwid.Text(' '),
          ('pack',urwid.Text('V1.0')),
        ])
        header=urwid.AttrWrap(header,'header')
        self.state=urwid.BigText('',font=self.statefont)
        self.progress_tot=urwid_timed_progress.TimedProgressBar('progress-normal','progress-complete',label='TOTAL:',      label_width=12,units='runs'  ,done=len(self.rc.configs))
        self.progress_run=urwid_timed_progress.TimedProgressBar('progress-normal','progress-complete',label='Current run:',label_width=12,units='events',done=1)
        progress=urwid.BoxAdapter(urwid.ListBox([
          self.progress_tot,
          self.progress_run,
        ]),height=4)
        self.producers={}
        self.collectors={}
        self.configs_list=urwid.Text('')
        main=urwid.ListBox([
          urwid.LineBox(urwid.Padding(self.state,'center',None),title='Status'),
          urwid.LineBox(progress,title='Progress'),
          urwid.LineBox(self.build_table(self.rc.dataproducers+self.rc.moreproducers,self.producers),title='Producers'),
          urwid.LineBox(self.build_table(self.rc.collectors+self.rc.loggers,self.collectors),title='Collector(s)'),
          urwid.LineBox(self.configs_list, title='Pending configs')
        ])
        return urwid.Frame(main,header=header,footer=footer)
    
    def build_table(self,connections,widgets):
        rows=[]
        widths=[
          (max(4,max(len(c   ) for c in connections            )),),
          (max(len(s[0]) for s in self.CONNECTION_STATES.values()),),
          (8,),
          (8,),
          ('weight',1)
        ]
        for connection in connections:
            row=[]
            for col in range(5):
                row.append(urwid.Text(''))
            row[0].set_text(connection)
            row[1].set_text('WAIT')
            widgets[connection]=row
            rows.append(urwid.Columns([w+(r,) for w,r in zip(widths,row)],dividechars=1))
        table=urwid.BoxAdapter(urwid.ListBox([
          urwid.Columns([w+(urwid.AttrWrap(r,'table-header'),) for w,r in zip(widths,[
              urwid.Text('NAME'                   ),
              urwid.Text('STATE'   ,align='center'),
              urwid.Text('DATA EV#',align='center'),
              urwid.Text('STAT EV#',align='center'),
              urwid.Text('MESSAGE'                ),
            ])],dividechars=1)
          ]+rows
        ),height=len(rows)+1)
        return table

class ITS3RunControl(pyeudaq.RunControl):
    def __init__(self,con,init_fname):
        pyeudaq.RunControl.__init__(self,con)
        self.connections={}
        self.status={}
        self.nsev={}
        self.ndev={}
        self.ReadInitialiseFile(init_fname)
        init=self.GetInitConfiguration()
        self.loggers      =list(filter(None,re.split('[ ,]+',init.Get('loggers'))))
        self.dataproducers=list(filter(None,re.split('[ ,]+',init.Get('dataproducers'))))
        self.moreproducers=list(filter(None,re.split('[ ,]+',init.Get('moreproducers'))))
        self.collectors   =list(filter(None,re.split('[ ,]+',init.Get('collectors'   ))))
        self.configs      =list(filter(None,re.split('[ ,]+',init.Get('configs'      ))))
        for text,n in [
            ('producers', len(self.dataproducers)+len(self.moreproducers)),
            ('collectors', len(self.collectors)),
            ('configs files', len(self.configs))]:
            if n==0:
                print(f"\nERROR! No {text} to run! Check the ini file!\n")
                exit()
        self.tui=ITS3TUI(self)
        self.lock=threading.Lock()
        self.running=False
        self.halt=False
    
    @exception_handler
    def DoConnect(self,c):
        with self.lock:
            self.connections[c.GetName()]=c
            self.status[c.GetName()]=None
            self.nsev[c.GetName()]=0
            self.ndev[c.GetName()]=0
            if c.GetName() in self.dataproducers+self.moreproducers:
                self.tui.update_producer(c.GetName(),0,0,0,'XXX')
            else:
                pass
    
    @exception_handler
    def DoDisconnect(self,c):
        with self.lock:
            del self.connections[c.GetName()]
            if c.GetName() in self.dataproducers+self.moreproducers:
                self.tui.update_producer(c.GetName(),0,0,0,'XXX')
    
    @exception_handler
    def DoStatus(self,c,s):
        with self.lock:
            self.status[c.GetName()]=s.GetState()
            self.nsev[c.GetName()]=int('0'+s.GetTag('StatusEventN'))
            self.ndev[c.GetName()]=int('0'+s.GetTag('DataEventN'  ))
            msg=s.GetMessage()
            if c.GetName() in self.dataproducers+self.moreproducers:
                self.tui.update_producer(c.GetName(),s.GetState(),self.ndev[c.GetName()],self.nsev[c.GetName()],msg)
            elif c.GetName() in self.collectors:
                self.tui.update_collector(c.GetName(),s.GetState(),self.ndev[c.GetName()],self.nsev[c.GetName()],msg)
            elif c.GetName() in self.loggers:
                self.tui.update_logger(c.GetName(),s.GetState(),msg)
    
    @exception_handler
    def framework_runner_(self):
        self.Exec()

    def check_status(self):
        with self.lock:
            in_error = [c for c,s in self.status.items() if s == pyeudaq.Status.STATE_ERROR]
        for c in in_error:
            pyeudaq.EUDAQ_ERROR(f"{c} in ERROR state")
        if in_error:
            raise RuntimeError("One or more conections in ERROR state.")

    def wait_replicas(self,state):
        wait=True
        while wait:
            wait=False
            for logger in self.loggers:
                with self.lock:
                    if not logger in self.status or self.status[logger]!=state:
                        wait=True
                        break
            for producer in self.dataproducers+self.moreproducers:
                with self.lock:
                    if not producer in self.status or self.status[producer]!=state:
                        wait=True
                        break
            for collector in self.collectors:
                with self.lock:
                    if not collector in self.status or self.status[collector]!=state:
                        wait=True
                        break
            sleep(0.01)
            self.check_status()

    def userlogic_wrapper(self):
        try:
            self.userlogic_runner_()
        except Exception as e:
            pyeudaq.EUDAQ_ERROR(str(e))
            pyeudaq.EUDAQ_ERROR(traceback.format_exc())
            self.tui.set_state('ERROR')
            if self.running:
                self.StopRun()

    def userlogic_runner_(self):
        self.tui.set_state('CONWAIT')
        self.wait_replicas(pyeudaq.Status.STATE_UNINIT)
        if self.loggers:
            pyeudaq.EUDAQ_LOG_CONNECT("RunControl","RC-TUI","tcp://localhost:55000") # TODO make nicer
        self.Initialise()
        self.wait_replicas(pyeudaq.Status.STATE_UNCONF)
        self.tui.set_state('UNCONF')
        for i,c in enumerate(self.configs):
            self.tui.update_configs_list(self.configs[i:])
            self.tui.reset_progress_run()
            self.ReadConfigureFile(c)
            self.Configure()
            self.wait_replicas(3)
            self.tui.set_state('CONF')
            self.StartRun()
            self.wait_replicas(5)
            self.tui.set_state('RUNNING')
            ntarget=int(self.GetConfiguration().Get('NEVENTS'))
            self.tui.target_progress_run(ntarget)
            nlast=0
            with self.lock: self.running = True
            while self.running:
                with self.lock: # locking might not be needed, to check
                    n=min(self.ndev[p] for p in self.dataproducers)
                self.tui.update_progress_run(n-nlast)
                nlast=n
                if n>=ntarget:
                    with self.lock:
                        self.running = False
                if not self.running:
                    break
                self.check_status()
                sleep(0.1)
            self.tui.update_progress_tot()
            self.StopRun()
            self.wait_replicas(pyeudaq.Status.STATE_STOPPED)
            self.tui.set_state('STOPPED')
            if self.halt: break
        self.tui.set_state('TERMINATED')
        self.Terminate()
        with self.lock: self.halt = True

    def start(self):
        self.framework_runner=threading.Thread(target=self.framework_runner_)
        self.userlogic_runner=threading.Thread(target=self.userlogic_wrapper)
        self.framework_runner.start()
        self.userlogic_runner.start()
        
    def join(self):
        self.userlogic_runner.join()
        self.framework_runner.join()
       
if __name__=='__main__':
    parser=argparse.ArgumentParser(description='ITS3 Run Control',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--address',default='44000')
    parser.add_argument('--ini',default='ITS3.ini')
    args=parser.parse_args()

    rc=ITS3RunControl(args.address,args.ini)
    rc.start()
    rc.tui.run()

