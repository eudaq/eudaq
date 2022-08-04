#!/usr/bin/env python3

import argparse

from numpy.typing import _128Bit
import pyeudaq
import numpy as np
import re
from datetime import datetime
import matplotlib.pyplot as plt


parser=argparse.ArgumentParser(description='ALPIDE status events plot')
parser.add_argument('filename')
parser.add_argument('-p', type=str, default='0', help='Plane number to look at' )
args=parser.parse_args()

fr=pyeudaq.FileReader('native',args.filename)

timesALPIDE,idda,iddd,tempALP = ([] for i in range(4))
timesPTH,pres,hum,tempPTH = ([] for i in range(4))
timesPOWER,iMeas,vMeas = ([] for i in range(3))

for _ in range(1000000): # cannot use counter with fr.GetNextEvent(), so chose a large number of events
    ev=fr.GetNextEvent()
    if ev is None: break
    if ev.GetDescription()=='ALPIDE_plane_'+args.p+'_status':
        idda.append(float(re.findall(r"[-+]?\d*\.\d+|\d+", ev.GetTag("IDDA"))[0]))
        iddd.append(float(re.findall(r"[-+]?\d*\.\d+|\d+", ev.GetTag("IDDD"))[0]))
        tempALP.append(float(re.findall(r"[-+]?\d*\.\d+|\d+", ev.GetTag("Temperature"))[0]))
        timesALPIDE.append(datetime.fromisoformat(ev.GetTag("Time")))
    if ev.GetDescription()=='PTH_status':
        pres.append(float(re.findall(r"[-+]?\d*\.\d+|\d+", ev.GetTag("Pressure"))[0]))
        hum.append(float(re.findall(r"[-+]?\d*\.\d+|\d+", ev.GetTag("Humidity"))[0]))
        tempPTH.append(float(re.findall(r"[-+]?\d*\.\d+|\d+", ev.GetTag("Temperature"))[0]))
        timesPTH.append(datetime.fromisoformat(ev.GetTag("Time")))
    if ev.GetDescription()=='POWER_status':
        timesPOWER.append(datetime.fromisoformat(ev.GetTag("Time")))
        i,v=[],[]
        for ch in range(1,5):
            i.append(float(re.findall(r"[-+]?\d*\.\d+|\d+", ev.GetTag("current_meas_"+str(ch)))[0]))
            v.append(float(re.findall(r"[-+]?\d*\.\d+|\d+", ev.GetTag("voltage_meas_"+str(ch)))[0]))
        iMeas.append(i)
        vMeas.append(v)

# ALPIDE status plotting
fig1, axes = plt.subplots(nrows=2, ncols=3, figsize=(20, 8))
fig1.autofmt_xdate(rotation=45)

axes[0][0].set_ylabel ('ALPIDE IDDA [mA]')
axes[0][0].plot(np.asarray(timesALPIDE),np.asarray(idda))
axes[0][1].set_ylabel ('ALPIDE IDDD [mA]')
axes[0][1].plot(np.asarray(timesALPIDE),np.asarray(iddd))
axes[0][2].set_ylabel ('ALPIDE Temp [C]')
axes[0][2].plot(np.asarray(timesALPIDE),np.asarray(tempALP))
fig1.tight_layout()

# PTH status plotting
axes[1][0].set_xlabel ('Time [h:m:s]')
axes[1][0].set_ylabel ('PTH Pressure [mbar]')
axes[1][0].ticklabel_format(useOffset=False)
axes[1][0].plot(np.asarray(timesPTH),np.asarray(pres))
axes[1][1].set_xlabel ('Time [h:m:s]')
axes[1][1].set_ylabel ('PTH Humidity [rel%]')
axes[1][1].plot(np.asarray(timesPTH),np.asarray(hum))
axes[1][2].set_xlabel ('Time [h:m:s]')
axes[1][2].set_ylabel ('PTH Temp [C]')
axes[1][2].plot(np.asarray(timesPTH),np.asarray(tempPTH))
fig1.tight_layout()

# POWER status plotting
fig2, axes = plt.subplots(nrows=2, ncols=4, figsize=(20, 8))
fig2.autofmt_xdate(rotation=45)

axes[0][0].set_ylabel ('HAMEG I [mA]')
axes[1][0].set_ylabel ('HAMEG V [V]')
for ch in range(1,5):
    axes[1][ch-1].set_xlabel ('Time [h:m:s]')
    axes[0][ch-1].plot(np.asarray(timesPOWER),np.asarray(iMeas)[:,ch-1], label='ch%d'%ch)
    axes[1][ch-1].plot(np.asarray(timesPOWER),np.asarray(vMeas)[:,ch-1], label='ch%d'%ch)
    axes[0][ch-1].legend(loc="upper right")
    axes[1][ch-1].legend(loc="upper right")
fig1.tight_layout()

plt.show()