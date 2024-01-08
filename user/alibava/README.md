Simple Producer which calls external Alibava GUI
=====

Has support for dedicated external HV control programm to support scanning voltages. HV is changed and pedestal is run during Config step. Pedestal file name contains run number + voltage. Normal output just runnumber.  First pedestal run after runcontrol startup assumes run number 1. 

### Config paramters:

[Producer.my_ali]
- ```HV = -100```
- ```HV_PROG = "/home/x/bin/hvcontrol"```
- ```ALIBAVA_EVENTS = 100000```
- ```ALIBAVA_PED_EVENTS = 10000```
- ```ALIBAVA_INI = "/home/x/alibava/config.ini"```
- ```ALIBAVA_OUT = "/home/x/alibava/data/"```
- ```ALIBAVA_PROG = "/opt/Alibava/AlibavaGUI/Contents/Linux/alibava-gui"```
