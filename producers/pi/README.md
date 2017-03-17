Readme for PIControl (CLI) and PIController (EUDAQ Controller)
==============================================================

### PIControl

PIControl is the command-line tool for testing the travel range 
and initialising the stages, e.g.

```
PIControl.exe.exe -ho 192.168.22.5 -p 50000 -s1 M-511.DD1 -s2 M-521.DD1 -s3 M-060.DG
```
Referencing is only done if needed, e.g. after power off of PI Controller 884.
Referencing includes assigning, switching on servo and moving to middle position.

```
PIControl.exe.exe -h
PI stage Control Utility version 1.1
A comand-line tool for controlling the PI stage

usage: PIControl.exe.exe [options]

options:
  -ho --HostName <string>       (default = 192.168.22.5)
     set hostname for TCP/IP connection
  -p --PortNumber <number>      (default = 50000)
     set portnumber for TCP/IP connection
  -s1 --StageName <string>      (default = NOSTAGE)
     set stage for axis 1
  -s2 --StageName <string>      (default = NOSTAGE)
     set stage for axis 2
  -s3 --StageName <string>      (default = NOSTAGE)
     set stage for axis 3
  -s4 --StageName <string>      (default = NOSTAGE)
     set stage for axis 4
  -v1 --Velocity1 <number>      (default = -1)
     set velocity of axis1, -1 no change
  -v2 --Velocity2 <number>      (default = -1)
     set velocity of axis2, -1 no change
  -v3 --Velocity3 <number>      (default = -1)
     set velocity of axis3, -1 no change
  -v4 --Velocity4 <number>      (default = -1)
     set velocity of axis4, -1 no change
  -p1 --Position1 <number>      (default = -1)
     move stage on axis1 to position, -1 no movement
  -p2 --Position2 <number>      (default = -1)
     move stage on axis2 to position, -1 no movement
  -p3 --Position3 <number>      (default = -1)
     move stage on axis3 to position, -1 no movement
  -p4 --Position4 <number>      (default = -1)
     move stage on axis4 to position, -1 no movement
```

### PIController

PIController is the EUDAQ component based on a Controller.
The DataCollector doesn't wait for data from Controller in contrast to a Producer.
Example init- and config-files are attached. Moving the stages happens during Configuration.
Generating config files and using the RunControl option, NextConfigFileOnFileLimit = 1, allow scans of mounted DUTs automatically.


### Installing

Building and compiling, only tested and available for Windows due to the PI-dll (PI_GCS2_DLL)
MSBUILD.exe INSTALL.vcxproj /p:Configuration=Release