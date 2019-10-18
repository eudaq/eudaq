EUDAQ version 2
=====

[![Build Status](https://travis-ci.com/eudaq/eudaq.svg?branch=master)](https://travis-ci.com/eudaq/eudaq)
[![Build status](https://ci.appveyor.com/api/projects/status/n3tq45kkupyvjihg/branch/master?svg=true)](https://ci.appveyor.com/project/eudaq/eudaq/branch/master)

EUDAQ is a Generic Multi-platform Data Acquisition Framework.
Version 2 comes with more flexible cross connectivity between components, multiple data collectors, and a cleaner seperation between core functionalities and user modules. 

### License

This program is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

### Documentation and context

Please check out the online documentation at 
- http://eudaq.github.io/ or 
- the manual, see below
- (https://telescopes.desy.de/EUDAQ for the use with the EUDET-type beam telescopes)

### Sources of documentation within the framework

The user's manual is provided as LaTeX source files in the repository;
to generate the pdf on Linux/OSX, make sure that you have installed ImageMagick, then follow these steps:
```
cd build
cmake -D EUDAQ_BUILD_MANUAL=ON ..
make pdf
```
The manual can then be found in ```./doc/manual/EUDAQUserManual.pdf```.

To generate the doxygen documentation, make sure that you have installed doxygen and run
```
make doxygen
```
in the ```build``` directory after CMake. The resulting HTML files are stored in ```../doc/doxygen/html```.


## Prerequisites

### For the core Library, Executables and GUI
EUDAQ requires a C++11 compliant compiler and a Qt version 5 or higher to build [GUIs](gui/README.md).
We recommend a gcc version 4.9 or later.
ROOT 6 is required for the Online Monitor GUI.

### User projects and modules
- Dummy: Skeletons to add user modules - not to be changed. Required by
[addModule.sh](etc/addModule.sh), that creates your module for you
- [example](user/example/README.md): run the example without hardware, see below Execution
- [eudet](user/eudet/README.md): EUDET-type beam telescopes and EUDET and AIDA TLU, required: Cactus/Ipbus Software for AIDA TLU and ZestSC1+Tlufirmware+Libusb for EUDET TLU
- [aidastrip](user/aidastrip/): SiStrip telescope for DESY TB24 (AIDA2020 WP15 development)
- [calice](user/calice/README.md): test beam user
- [itkstrip](user/itkstrip/README.md): test beam user
- [stcontrol](user/stcontrol/README.md): USBPix/FEI4 
- [tbscDESY](user/tbscDESY/README.md): Slow Control System at DESY test beam
- [timepix3](user/timepix3/README.md): Timepix3 read out, required: Spidr
- [tlu](user/tlu/README.md): folder for Trigger Logic Units: EUDET and AIDA TLU
- [torch](user/torch/README.md): test beam user
- MuPix8: No readme given, as only converter
- [PI Stages](user/piStage/README.md): Producer to control the rotation and translation stages from PI
- [experimental](user/experimental/README.md): developed, not tested (with hardware)


## Compiling and installation

```cmake``` will configure the installation and prepare the makefiles. 
It searches for all the required files. 
As a standard, it is executed in the ```build``` folder. 
Since the relevant CMakeLists.txt is in the main level, thus, the command is ```cmake ..```. 
If cmake is successful, EUDAQ can be installed. 
Variables set are cached in CMakeCache.txt and will again be taken into account at the next cmake run.

cmake has several options (```cmake -D OPTION=ON/OFF ..```) to activate or deactivate programs which will be built, here printed with their default value:  
- ```EUDAQ_BUILD_EXECUTABLE=ON```
- ```EUDAQ_BUILD_GUI=ON```
- ```EUDAQ_BUILD_DOXYGEN=OFF```
- ```EUDAQ_BUILD_MANUAL=OFF```
- ```EUDAQ_BUILD_PYTHON=OFF```
- ```EUDAQ_BUILD_STDEVENT_MONITOR=OFF```
- ```EUDAQ_EXTRA_BUILD_NREADER=OFF```
- ```EUDAQ_LIBRARY_BUILD_LCIO=OFF```
- ```EUDAQ_LIBRARY_BUILD_TTREE=OFF```
- ```USER_AIDASTRIP=OFF```
- ```USER_CALICE_BUILD=OFF```
- ```USER_EUDET_BUILD=OFF```
- ```USER_EXAMPLE_BUILD=ON```
- ```USER_EXPERIMENTAL_BUILD=OFF```
- ```USER_ITKSTRIP_BUILD=OFF```
- ```USER_STCONTROL_BUILD=OFF```
- ```USER_TBSCDESY=OFF```
- ```USER_TLU_BUILD=OFF```
- ```USER_TIMEPIX3_BUILD=OFF```

If cmake is not successful and complains about something is missing, it is recommended to clean the ```build``` folder by ```rm -rf *``` before a new try.
If problems occur during installation, please have a look in the issues, if a similiar problem already occured. If not, feel free to create a new ticket: https://github.com/eudaq/eudaq/issues

### Quick installation for UNIX

Prerequisites for Ubuntu 18.04.01 LTS: 
```sudo apt install openssh-server git cmake build-essential qt5-default xterm zlib1g-dev```

Get and compile the code
```
git clone -b master https://github.com/eudaq/eudaq.git
mkdir -p eudaq/build
cd eudaq/build
cmake ..
make install
```

### Notes for Windows

#### Visual Studio for compiling (MSVC)

The recommended windows compiler is MSVC (Microsoft Visual C++) like Visual Studio 14 2015 and later: 
Download Visual Studio Express Desktop (e.g. 2015 Version) [here.](http://www.microsoft.com/en-us/download/details.aspx?id=40787)

#### Compiling using cmake syntax

Start the Visual Studio "Developer Command Prompt" from the Start Menu entries for Visual Studio (Tools subfolder) which opens a cmd.exe session with the necessary environment variables already set. 

If your Qt installation path has not been added to the global %PATH% variable, you need to execute the "qtenv2.bat" batch file in the Qt folder, e.g. and replace "5.1.1" with the version string of your Qt installation:
```
C:\Qt\Qt5.1.1\5.1.1\msvc2013\bin\qtenv2.bat
```
Go to the EUDAQ folder and configure, as describe above:
```
cd c:\[...]\eudaq\build
cmake ..
```
(Note: Please make sure the right compiler is found. You can select the right generator by ```cmake -G ...``` or by using the cmake-gui.)

This generates the VS project files. Installing into eudaq\bin by:
```
cmake --build . --target install --config Release
```

### Notes for OS X

- Compiler: Clang (at least version 3.1)
- Install Qt5 or later, e.g. by using MacPorts (http://www.macports.org/): sudo port install qt5-mac-devel


## Execution

In UNIX:
```
cd ../user/example/misc
./example_startrun.sh
```
The startrun script assembles the new command line syntax: Core executables are started by loading a specific module with the name option ```-n``` assigning a unique tag by the option ```-t```:
```
cd bin
./euRun &
sleep 1
./euLog &
sleep 1
./euCliMonitor -n Ex0Monitor -t my_mon &
./euCliCollector -n Ex0TgDataCollector -t my_dc &
./euCliProducer -n Ex0Producer -t my_pd0 &
./euCliProducer -n Ex0Producer -t my_pd1 &
```

Play around with Init, Configure, Start, Stop, Re-Start or Re-Configure and Start or Reset and Re-Init and repeat -- or Terminate. 

For Initialising and Configuring you have to Load to set the path to the the ini or conf file:
- ../eudaq/user/example/misc/Ex0.ini
- ../eudaq/user/example/misc/Ex0.conf

A description for operating the EUDET-type beam telescopes is under construction:
https://telescopes.desy.de/User_manual
