# CorryMonitor
The `CorryMonitor` is an online monitoring tool for the use at test beams. It was desigend to be versatile such that it can be used by a large user-base. See the [AIDAinnova Milestone 10 Report](https://zenodo.org/records/8402178) for a write-up of the `CorryMonitor` working principle and features. This document is meant to guide users through the example provided for the `CorryMonitor` in this repository.
The most imporant point from the Milestone Report, is that this monitor utilises the existing test beam data analysis program `Corryvreckan` for displaying of online monitoring plots.

## Prerequisites
Since `CorryMonitor` relies on the widely used `Corryvreckan` for plotting of the data, it goes without saying that a local installation of `Corryvreckan` is required. `Corryvreckan` itself has minimal external dependencies, requiring `EIGEN3` for fast matrix calcluations and `ROOT` for a variety of tasks. Please see the documentation on the [project website](https://project-corryvreckan.web.cern.ch/project-corryvreckan/page/about/) for details and user guides.
**Please note that for the `Corryvreckan` installation the `-DBUILD_EventLoaderEUDAQ2=ON` flag needs to be set to work with `CorryMonitor`.**


## Example Setup
The repository comes with a simple example to showcase how to use and prepare the required files for the `CorryMonitor`. Familiarity with the `Corryvreckan` framework certainly helps with preparing the files, however, users can also adapt the example files given in the repository to their needs. 
This example is closely aligned to the example setup in the `EUDAQ2` user manual.

### Example Scenario
This example uses the `Ex0Producer` and `Ex0TgDataCollector` provided by `EUDAQ2`. The `Ex0Producer` mimics data taking by generating randomly distributed data for a 16x16 pixel detector. The generated signal for each pixel is a random value between 0 and 255. These data are being sent to the `Ex0TgDataCollector` which is able to group data from various `Producers` by trigger number and write them to file. Since this example only uses one `Producer` the data are simply stored in a file following the file naming pattern specified in the `EUDAQ2` configuration file.

### Run the Example Scenario
To run the example scenario, simply `source corrymon_startrun.sh`. **Before initialising `EUDAQ2` with the file `CorryMon.ini` using the GUI application, the parameter  `CORRY_PATH` needs to be adapted to the absolute path to your locally installed `corry` binary file.** *Avoid using '~' at the beginning of the path as this can cause problems.*
`EUDAQ2` can then be initialised and configured with `CorryMon.ini` and `CorryMon.conf` respectively and the run can be started.

In the sections below we will discuss the involved files in more detail.

### Startup Script
Sourcing the `corrymon_startrun.sh` script starts the required `EUDAQ2` processes, in this case the `RunControl`, `LogCollector`, `CorryMonitor`, `Ex0Producer` and `Ex0TgDataCollector`.  The `RunControl` and `LogCollector` are each instanced by a GUI application.

### EUDAQ2 Initialisation and Configuration Files
Before the run can be started using the `RunControl` GUI the `.ini` and `.conf` files need to be loaded. For this example these are `CorryMon.ini` and `CorryMon.conf`.

##### CorryMon.ini:
For this example the important parameter is the `CORRY_PATH` in the `[Monitor]` section. It should contain the full absolute path to the locally installed `corry` binary file. Before running the example setup, this value needs to be changed to your installation path. Avoid using '~' at the beginning of the path as this can cause problems.

 ##### CorryMon.conf: 
 Here the `[Monitor]` section specifies paramters with which the `corry` software is called internally later on.

 - `CORRY_CONFIG_PATH` is the path and file to the `Corryvreckan` configuration file. More on this in the next section. This path can be the relative path.
 - `CORRY_OPTIONS`: Any options the user want to pass to `corry` can be placed here. This string is passed in its entirety to the `corry` call and should therefore be formatted in the same way as you would pass options to `corry` in the command line.
 - `DATACOLLECTORS_TO_MONITOR` is a comma separated list of `DataCollectors` the user wishes to monitor with `CorryMonitor`. As described earlier, the monitor uses the information from the `DataCollector` to find the file created by this colllector automatically.
 - `CORRESPONDING_EVENTLOADER_TYPES` is a comma separated list of the names for each`DataCollector`'s event type. In this example the event type created by the `Ex0Producer` for the `Ex0TgDataCollector` is `Ex0Raw`. The names are not case-sensitive.

### Corryvreckan Configuration File
Similarly to `EUDAQ2`, `Corryvreckan` requires a configuration file in which the modules which are to be loaded and their parameters need to be specified. A detailed description on how this configuration file needs to be structured can be found in the extensive documentation for `Corryvreckan`. Here we'll quickly cover each of the sections in `corryconfig.conf`.

##### [Corryvreckan]
This section defines global `Corryvreckan` parameters. The `detectors_file` is a required parameter and specifies the detector geometry. In the next section we will discuss the detector file for this example. 
`histogram_file` is the output file in which `Corryvreckan` histograms and plots are saved to.

##### [Metronome]
In this example the `Corryvreckan` event definition is done by the `Metronome` module. In most cases the user will use a different event definition than the one we use here. Again, see the extensive documentation for details on the event defintion.

##### [EventLoaderEUDAQ2]
The documentation for this module can be found in the `Corryvreckan` user manual.
The `type` parameter describes the event type which is to be loaded with this `EventLoaderEUDAQ2` module. Be aware, that each different event type needs its own `EventLoaderEUDAQ2` module, which is realised by creating a section `[EventLoaderEUDAQ2]` for each `type`.
The `file_name` is the name of the file from which data is to be read. Since `Corryvreckan` is usually used in an offline setting, the file name is known by the user in this scenarion and `file_name` is a required parameter. Since this is not the case in the online monitoring setting, this parameter is filled with a placeholder and overwritten by the `corry` call inside `CorryMonitor`. **The user therefore can leave this parameter as is.**
`wait_on_eof` needs to be set to `true` to prevent `corry` from closing when the end of file is reached. Since the monitoring is relatively quick it can occur that it will have to wait for new data to come in. **If there are multiple `EventLoaderEUDAQ2` modules, this flag needs to be set to true for each of them to prevent `corry` from closing upon reaching the end of any file read by the `EventLoaderEUDAQ2` modules**.

##### [OnlineMonitor]
This section is required to start the online monitoring GUI application which will display the plots specified in `dut_plots`, `hitmaps` and possibly other sections. See the user manual for more options.

### Corryvreckan Detectors File
The detectors file, in this example `corrygeo.geo`, specifies for each detector (plane) the geometry and its spatial relation to other planes, as well as the `type` of event this plane corresponds to. For all detectors the user wishes to monitor, the `role` should include "dut" such that all plots are easiliy accessible in the "DUT" section of the `OnlineMonitor` GUI. 
Note that there has to be *exactly* one "reference" plane.