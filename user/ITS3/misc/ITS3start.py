#!/usr/bin/env python3

import os
import time
import libtmux
import argparse
import configparser
from rich.prompt import Confirm
from rich import print

def setup_tmux(
    ini_path,
    nalpide=6,ndpts=1,napts=1,nopamp=1,npower=1,npth=1,nrtd23=1,nzaber=1,rclog="rc.log",
    dpts_utils_path=None,
    trigger_path=None):

    eudaq_path=os.path.abspath(os.path.dirname(__file__)+"/../../..")
    eudaq_lib_path=os.path.join(eudaq_path,"lib")
    producer_path=os.path.join(eudaq_path,"user/ITS3/python")
    rc_exe=    f"PYTHONPATH={eudaq_lib_path} {producer_path}/ITS3RunControl.py"
    dc_exe=    f"PYTHONPATH={eudaq_lib_path} {producer_path}/ITS3DataCollector.py"
    alpide_exe=f"PYTHONPATH={eudaq_lib_path} {producer_path}/ALPIDEProducer.py"
    dpts_exe=  f"PYTHONPATH={eudaq_lib_path}:{dpts_utils_path}:{trigger_path} {producer_path}/DPTSProducer.py"
    apts_exe=  f"PYTHONPATH={eudaq_lib_path} {producer_path}/APTSProducer.py"
    opamp_exe= f"PYTHONPATH={eudaq_lib_path} {producer_path}/OPAMPProducer.py"
    power_exe= f"PYTHONPATH={eudaq_lib_path} {producer_path}/PowerProducer.py"
    pth_exe=   f"PYTHONPATH={eudaq_lib_path} {producer_path}/PTHProducer.py"
    rtd23_exe=   f"PYTHONPATH={eudaq_lib_path} {producer_path}/RTD23Producer.py"
    zaber_exe=   f"PYTHONPATH={eudaq_lib_path} {producer_path}/ZABERProducer.py"
    logger_exe=f"{eudaq_path}/bin/euCliLogger"

    server = libtmux.Server()
    try:
        session = server.new_session(session_name="ITS3",window_name="rc")
    except libtmux.exc.TmuxSessionExists:
        kill = Confirm.ask("[bold red]ITS3 session exists! Do you want to kill it?[/bold red]")
        if not kill: exit()
        session = server.new_session(session_name="ITS3",window_name="rc",kill_session=True)

    session.set_option("pane-border-status","top",True)
    session.set_option("pane-border-format","#P: #{pane_title} (#{pane_pid})",True)

    server.cmd('select-pane','-t','ITS3:rc','-T','Run Control')

    window = session.new_window("ap")
    for i in range(1,nalpide):
        window.split_window()
        window.select_layout("tiled")
    for i in range(0,nalpide):
        server.cmd('select-pane','-t',f'ITS3:ap.{i}','-T',f'Producer ALPIDE {i}')

    window = session.new_window("op")
    for i in range(1,ndpts+napts+nopamp):
        window.split_window()
        window.select_layout("tiled")
    for i in range(0,ndpts):
        server.cmd('select-pane','-t',f'ITS3:op.{i}','-T',f'Producer DPTS {i}')
    for i in range(ndpts,ndpts+napts):
        server.cmd('select-pane','-t',f'ITS3:op.{i}','-T',f'Producer APTS {i-ndpts}')
    for i in range(ndpts+napts,ndpts+napts+nopamp):
        server.cmd('select-pane','-t',f'ITS3:op.{i}','-T',f'Producer OPAMP {i-ndpts-napts}')

    window = session.new_window("sp")
    for i in range(1,npower+npth+nrtd23+nzaber):
        window.split_window()
        window.select_layout("tiled")
    for i in range(0,npower):
        server.cmd('select-pane','-t',f'ITS3:sp.{i}','-T',f'Producer POWER {i}')
    for i in range(npower,npower+npth):
        server.cmd('select-pane','-t',f'ITS3:sp.{i}','-T',f'Producer PTH {i-npower}')
    for i in range(npower+npth,npower+npth+nrtd23):
        server.cmd('select-pane','-t',f'ITS3:sp.{i}','-T',f'Producer RTD23 {i-(npower+npth)}')
    for i in range(npower+npth+nrtd23,npower+npth+nrtd23+nzaber):
        server.cmd('select-pane','-t',f'ITS3:sp.{i}','-T',f'Producer ZABER {i-(npower+npth+nzaber)}')

    session.new_window("dc")
    server.cmd('select-pane','-t','ITS3:dc','-T','Data Collector')

    p=session.new_window("log").select_pane("ITS:log")
    server.cmd('select-pane','-t','ITS3:log','-T','EUDAQ Log')

    p=session.new_window("perf").select_pane("ITS3:perf")
    server.cmd('select-pane','-t','ITS3:perf','-T','Performance')
    p.send_keys('htop')

    session.select_window("rc").select_pane("ITS3:rc").send_keys(f'{rc_exe} --ini {ini_path} 2>> {rclog}')
    time.sleep(1)
    session.select_window("log").select_pane("ITS3:log").send_keys(f'{logger_exe} -n log -a 55000')
    session.select_window("dc").select_pane("ITS3:dc").send_keys(f'{dc_exe}')
    for i in range(nalpide):
        session.select_window("ap").select_pane(f"ITS3:ap.{i}").send_keys(f'{alpide_exe} --name ALPIDE_plane_{i}')
    for i in range(0,ndpts):
        session.select_window("op").select_pane(f"ITS3:op.{i}").send_keys(f'{dpts_exe} --name DPTS_{i}')
    for i in range(ndpts,ndpts+napts):
        session.select_window("op").select_pane(f"ITS3:op.{i}").send_keys(f'{apts_exe} --name APTS_{i-ndpts}')
    for i in range(ndpts+napts,ndpts+napts+nopamp):
        session.select_window("op").select_pane(f"ITS3:op.{i}").send_keys(f'{opamp_exe} --name OPAMP_{i-ndpts-napts}')
    for i in range(0,npower):
        session.select_window("sp").select_pane(f'ITS3:sp.{i}').send_keys(f'{power_exe} --name POWER_{i}')
    for i in range(npower,npower+npth):
        session.select_window("sp").select_pane(f'ITS3:sp.{i}').send_keys(f'{pth_exe} --name PTH_{i-npower}')
    for i in range(npower+npth,npower+npth+nrtd23):
        session.select_window("sp").select_pane(f'ITS3:sp.{i}').send_keys(f'{rtd23_exe} --name RTD23_{i-(npower+npth)}')
    for i in range(npower+npth+nrtd23,npower+npth+nrtd23+nzaber):
        session.select_window("sp").select_pane(f'ITS3:sp.{i}').send_keys(f'{zaber_exe} --name ZABER_{i-(npower+npth+nrtd23)}')

    if server.cmd('switch-client','-t','ITS3:rc').stderr == ['no current client']:
        session.cmd('attach-session','-t','ITS3:rc')
    

def parse_ini(ini_path):
    conf = configparser.ConfigParser()
    conf.read(ini_path)
    producers = conf.get("RunControl","dataproducers",fallback="").split("#")[0] +","+\
        conf.get("RunControl","moreproducers",fallback="").split("#")[0]
    ret = {"n"+prod.lower(): producers.count(prod+"_") \
        for prod in ["ALPIDE","DPTS","APTS","OPAMP","POWER","PTH","RTD23","ZABER"]}
    ret["dpts_utils_path"] = conf.get("LibraryPaths","dpts_utils_path",fallback=None)
    ret["trigger_path"]    = conf.get("LibraryPaths","trigger_path",fallback=None)
    return ret


if __name__=="__main__":
    parser = argparse.ArgumentParser("ITS3 EUDAQ2 startup script")
    parser.add_argument('ini_path',help="EUDAQ2 INI file.")
    parser.add_argument('--rclog',default="rc.log",help="Run Control log file (default=rc.log).")
    parser.add_argument('--dpts-utils-path',help="Path to 'dpts-utils' repository, needed if not provided in INI.")
    parser.add_argument('--trigger-path',help="Path to 'software' directory in 'trigger' repository, needed if not provided in INI.")
    for prod in ["ALPIDE","DPTS","APTS","OPAMP","POWER","PTH","RTD23","ZABER"]:
        parser.add_argument('--n'+prod.lower(),type=int,help=f"Override the number of {prod} producers to start.")
    args = parser.parse_args()

    ini_args = parse_ini(args.ini_path)
    ini_args.update({k:v for k,v in vars(args).items() if v is not None})
    print("Starting EUDAQ with following arguments:", ini_args)
    setup_tmux(**ini_args)