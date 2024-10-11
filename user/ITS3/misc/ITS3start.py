#!/usr/bin/env python3

import os
import time
import libtmux
import argparse
import configparser
from rich.prompt import Confirm
from rich import print

REF_PRODUCERS = ["ALPIDE"]
DUT_PRODUCERS = ["DPTS","APTS","OPAMP","MOSS"]
STATUS_PRODUCERS = ["Trigger","Power","PTH","RTD23","ZABER"]
PRODUCERS = REF_PRODUCERS+DUT_PRODUCERS+STATUS_PRODUCERS

class Executable:
    def __init__(self,cmd, args="", plane_name=None):
        self.cmd = cmd
        self.args = args
        self.plane_name = plane_name

def setup_tmux(
    ini_path: str, n_producers: dict, rclog="rc.log", python_exe="python3"):

    eudaq_path=os.path.abspath(os.path.dirname(__file__)+"/../../..")
    eudaq_lib_path=os.path.join(eudaq_path,"lib")
    exe_path=os.path.join(eudaq_path,"user/ITS3/python")
    prefix = f"PYTHONPATH={eudaq_lib_path} {python_exe} {exe_path}/"

    main_exes = {
        "rc"  : Executable(prefix+"ITS3RunControl.py",  args=f"--ini {ini_path} 2>> {rclog}"),
        "dc"  : Executable(prefix+"ITS3DataCollector.py"),
        "log" : Executable(f"{eudaq_path}/bin/euCliLogger", args="-n log -a 55000"),
        "perf": Executable( "htop")
    }
    ref_exes = {
        name: Executable(prefix+name+"Producer.py",plane_name=name+"_plane") for name in REF_PRODUCERS
    }
    status_exes = {
        name: Executable(prefix+name+"Producer.py",plane_name=name.upper()) for name in STATUS_PRODUCERS
    }
    dut_exes = {
        name: Executable(prefix+name+"Producer.py",plane_name=name.upper()) for name in DUT_PRODUCERS
    }

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

    for window_name,producer_list in [
        ("rp", REF_PRODUCERS),
        ("dp", DUT_PRODUCERS),
        ("sp", STATUS_PRODUCERS)
    ]:
        window = session.new_window(window_name)
        for i in range(1,sum(n_producers[name] for name in producer_list)):
            window.split_window()
            window.select_layout("tiled")
        j = 0
        for name in producer_list:
            for i in range(0,n_producers[name]):
                server.cmd("select-pane","-t",f"ITS3:{window_name}.{j}","-T",f"Producer {name} {i}")
                j += 1

    session.new_window("dc")
    server.cmd('select-pane','-t','ITS3:dc','-T','Data Collector')
    session.new_window("log").select_pane("ITS:log")
    server.cmd('select-pane','-t','ITS3:log','-T','EUDAQ Log')
    session.new_window("perf").select_pane("ITS3:perf")
    server.cmd('select-pane','-t','ITS3:perf','-T','Performance')
    for name,exe in main_exes.items():
        session.select_window(name).select_pane(f"ITS3:{name}").send_keys(f'{exe.cmd} {exe.args}')
        if name == "rc": time.sleep(1)

    for window_name,exe_list in [
        ("rp", ref_exes),
        ("dp", dut_exes),
        ("sp", status_exes)
    ]:
        j=0
        for name,exe in exe_list.items():
            for i in range(0,n_producers[name]):
                session.select_window(window_name).select_pane(f"ITS3:{window_name}.{j}").send_keys(
                    f"{exe.cmd} --name {exe.plane_name}_{i} {exe.args}")
                j += 1

    if server.cmd('switch-client','-t','ITS3:rc').stderr == ['no current client']:
        session.cmd('attach-session','-t','ITS3:rc')
    

def parse_ini(ini_path):
    conf = configparser.ConfigParser()
    conf.read(ini_path)
    producers = conf.get("RunControl","dataproducers",fallback="").split("#")[0] +","+\
        conf.get("RunControl","moreproducers",fallback="").split("#")[0]
    ret = {"n_producers": {prod: producers.count(prod.upper()+"_") for prod in PRODUCERS}}
    log_file_path = conf.get("LogCollector.log", "FILE_PATTERN",fallback=None)
    if log_file_path is not None:
        log_dir = os.path.abspath(os.path.dirname(log_file_path))
        if not os.path.exists(log_dir):
            print(f"Log directory specified in ini file does NOT exist. Creating: {log_dir}")
            os.makedirs(log_dir)
    return ret


if __name__=="__main__":
    parser = argparse.ArgumentParser("ITS3 EUDAQ2 startup script")
    parser.add_argument('ini_path',help="EUDAQ2 INI file.")
    parser.add_argument('--rclog',default="rc.log",help="Run Control log file (default=rc.log).")
    parser.add_argument('--python-exe', '-p', default="python3",help="Python executable to use")
    args = parser.parse_args()

    ini_args = parse_ini(args.ini_path)
    ini_args.update({k:v for k,v in vars(args).items() if v is not None})
    print("Starting EUDAQ with following arguments:", ini_args)
    setup_tmux(**ini_args)
