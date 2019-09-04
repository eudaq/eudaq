## EUDAQ version 2 Graphical User Interface

This README section features 2 GUIs: A `LogCollecor` GUI and a main `RunControl GUI`.

## RunControl GUI

The RunControl GUI provides an graphical overview over the DAQ System, including all producers, dataCollectors, Monitors and LogCollectors (Components in the following)
The GUI consists of three sections:

#### State
Shows the overall state of the DAQ. The component with the lowest state defines the system state. The possible states are
  - ERROR: Something went wrong
  - UNINIT: The startup state - Init required to continue
  - UNCONF: Initialized but not yet configured
  - CONF: Configured - ready to take data
  - STOPPED: Data taking has been stopped and the system has not been
  reconfigured afterwards. 
  - RUNNING: Actively running

#### Control

Buttons and line edits to control the DAQ. Inactive buttons are greyed out:
  - Init file:[ExampleFile](../user/example/misc/Ex0.ini)
    - Line edit for the currently set init file, which can be changed by simply entering the desired filename (NB: has to have the end `.ini`)
    - Load-Button: Opens Dialog to browse the system searching for a `*.ini` file, which is loaded to the line-edit
    - Init-Button: Distibutes the init command and file to all connected components
  - Config File: [ExampleFile](../user/example/misc/Ex0.conf)
    - Line edit for the currently set configuration file, which can be changed by simply entering the desired filename (NB: has to have the end `.conf`)
    - Load-Button: Opens Dialog to browse the system searching for a `*.conf` file, which is loaded to the line-edit
    - Config-Button: Distibutes the configure command and file to all connected components
  - Next RunN: Can be set if the next run is not supposed to be the last number + 1
  - Start-Button: Start data taking
  - Stop-Button: Stop data taking
  - Blue progress bar: Shows completion of the current scan - cannot be changed manually
  - Scan File: [ExampleFile](../user/example/misc/scan/ExampleScan.scan) Create a scan of any parameter, detailed description and examples can be found below
    - Line edit for the currently set scan configuration file, which can be changed by simply entering the desired filename (NB: has to have the end `.scan`)
    - Load-Button: Opens Dialog to browse the system searching for a `*.scan` file, which is loaded to the line-edit
    - StartScan-Button: Starts the scan and allows for an interruption at any time.
  
#### Connections 
All connected components are shown in a table located in the bottom part. For each component, the following information is displayed:
  - type: Can be Producer, DataCollector, Monitor
  - name: Registered Name
  - state: Current state
  - Connection: tcp:://IP:Port 
  - message: Last status message sent
  - information: frequently updated detailed information about the component.
  
Right clicking on each component opens a dialog to set the component into a desired `state`. 
Note that most components can only go up one state per step.
  
##  Log GUI
  
Self explaining GUI to scroll through the logs.
  
### Configure a scan
   
Each `*.scan` file needs a [global] section which defines the scan behavior. Any actual scan is defined in a numbered section [0..1000]. 
Scans sections with number above 1000/ below 0 are not considered.
A running example of a scan can be found [here](../user/example/misc/scan), including starting scripts and all required files.
The scans are executed according to their assigned number [i] is always executed before [i+1]. 
A scan can be defined to be `nested`. If so, all previous configurations are repeated for each point in the other scan:
[0] from s1=2 to s1=3 (with default =2) and [1] from s2=8 to s2=10 (with default =8) will be in pairs (s1,s2)

```nested = false: (2,8)->(3,8)->(2,9)->(2,10)```

```nested = true: (2,8)->(3,8)->(2,9)->(3,9)->(2,10)->(3,10)```

 The [global]- section has the following parameters:
 - `configPrefix`: The prefix for all files: _SCANSTEP.scan is added automatically - Default:`default`
 - `timeBasedScan`: `1` if the scan should take a certain time at each point, else a number of events is taken at each step.
  Default:`1`
 - `timePerStep`: Run-time in `s` for each step, no default value and only required if `timeBasedScan == 1`
 - `nEventsPerStep`: Number of events per step, no default value and only required if `timeBasedScan != 1`
 - `allowNested`: If set to `1` nested scans are allowed, overwrites section `nested` argument. Default `0` 
 - `repeatScans `: Repeat the scan until the user interrupts if set to `1`. Otherwise stop after finishing. Default: `0`
 
 Each scan section [0..1000] requires the parameters:
 - `parameter`: The parameter that should be scanned
 - `start`: Start point of scan
 - `stop`: Stop point of scan
 - `step`: Step size of scan
 - `name`: Name of the component containing the scanned parameter
 - `default`: Default value to be used in all scan section [i!=iScan]. Default:`start`
 - `nested`: Repeat all scan sections with [i<iScan] for each step if set to `1`, no effect otherwise. Default:`0`
 - `eventCounter`: Required if [global.timeBasedScan!=1]. 
 Defines the component that triggers the next step, if it has seen [gloabl.neventsPerStep] events.
