EUDAQ version 1
=======

[![Build Status](https://travis-ci.org/eudaq/eudaq.svg?branch=v1.7-dev)](https://travis-ci.org/eudaq/eudaq) 
[![Build status](https://ci.appveyor.com/api/projects/status/n3tq45kkupyvjihg/branch/v1.6-dev?svg=true)](https://ci.appveyor.com/project/eudaq/eudaq/branch/v1.7-dev)

**Note: EUDAQ version 1.8 requires a C++14 compliant compiler.**

EUDAQ is a Generic Multi-platform Data Acquisition Framework.
Version 1 uses a central data collector which synchronize multiple data streams by event number. 
This works smoothly with the EUDET TLU.

### License
This program is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

### First steps

Please check out the online documentation at 
- http://eudaq.github.io/ or 
- the manual, see below
- (https://telescopes.desy.de/EUDAQ for the use with the EUDET-type beam telescopes)

### Other sources of documentation

The user's manual is provided as LaTeX source files in the repository;
to generate the pdf on Linux/OSX, follow these steps:
```
cd build
cmake -DBUILD_manual=ON ..
make pdf
```
The manual can then be found in ```./doc/manual/EUDAQUserManual.pdf```.

To generate the doxygen documentation, make sure that you have installed doxygen and run
```
make doxygen
```
in the ```build``` directory after CMake. The resulting HTML files are stored in ```../doc/doxygen/html```.

## Prerequisites for installation

### For the core and executables

EUDAQ requires a C++14 compliant compiler. 
We recommend a gcc version 5.0 or later. 
Qt version 4/5 or higher is required to build GUIs. 
ROOT 5/6 is required for the Online Monitor GUI.

### For specific Producers and Components

#### EUDET TLU producer

- install **ZestSC1** driver package (if AFS is accessible on the machine, this will be installed automatically when running CMake; otherwise, manually copy full folder with sub-directories from /afs/desy.de/group/telescopes/tlu/ZestSC1 to into ./extern subfolder in EUDAQ sources)
- install **tlufirmware** bitfiles (if AFS is accessible on the machine, this will be installed automatically when runnig CMake; otherwise, manually copy /afs/desy.de/group/telescopes/tlu/tlufirmware into ./extern)
- libusb: 
-- Windows: install libusb (download from http://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/, for documentation see http://sourceforge.net/apps/trac/libusb-win32/wiki) into ./extern/libusb-w32
-- Linux: install libusb development package, e.g. for Ubuntu ```sudo apt install libusb-dev```


## Compiling and Installtion

```cmake``` will configure the installation and prepare the makefiles. 
It searches for all the required files. 
As a standard, it is executed in the ```build``` folder. 
Since the relevant CMakeLists.txt is in the main level, thus, the command is ```cmake ..```. 
If cmake is successful, EUDAQ can be installed. 
Variables set are cached in CMakeCache.txt and will again be taken into account at the next cmake run.

cmake has several options (```cmake -D OPTION=ON/OFF ..```) to activate or deactivate programs which will be built, here printed with their default value:

- ```BUILD_AHCAL=OFF```
- ```BUILD_BIFAHCAL=OFF```
- ```BUILD_TESTING=ON```
- ```BUILD_WITH_QT4=OFF```
- ```BUILD_allproducer=OFF```
- ```BUILD_altro=OFF```
- ```BUILD_altroUSB=OFF```
- ```BUILD_cmspixel=OFF```
- ```BUILD_depfet=OFF```
- ```BUILD_drs4=OFF```
- ```BUILD_fortis=OFF```
- ```BUILD_gui=ON```
- ```BUILD_main=ON```
- ```BUILD_manual=OFF```
- ```BUILD_mimoroma=OFF```
- ```BUILD_miniTLU=OFF```
- ```BUILD_ni=ON```
- ```BUILD_nreader=OFF```
- ```BUILD_offlinemon=OFF```
- ```BUILD_onlinemon=ON```
- ```BUILD_palpidefs=OFF```
- ```BUILD_pi=OFF```
- ```BUILD_pixelmanproducer=OFF```
- ```BUILD_python=OFF```
- ```BUILD_root=OFF```
- ```BUILD_rpi-controller=OFF```
- ```BUILD_runsplitter=ON```
- ```BUILD_taki=OFF```
- ```BUILD_timepix3=OFF```
- ```BUILD_timepixdummy=OFF```
- ```BUILD_tlu=OFF```

If cmake is not successful and complains about something is missing, it is recommended to clean the ```build``` folder by ```rm -rf *``` before a new try.
If problems occur during installation, please have a look in the issues, if a similiar problem already occured. If not, feel free to create a new ticket: https://github.com/eudaq/eudaq/issues


### Quick installation

```
git clone -b v1.7-dev https://github.com/eudaq/eudaq.git
mkdir -p eudaq/build
cd eudaq/build
cmake ..
make install
```

### Notes for Windows

#### Visual Studio for compiling (MSVC)

- The recommended windows compiler is MSVC (Microsoft Visual C++) like Visual Studio 2013 and later
- Download Visual Studio Express Desktop (e.g. 2013 Version): http://www.microsoft.com/en-us/download/details.aspx?id=40787

#### Compiling using cmake syntax

Start the Visual Studio "Developer Command Prompt" from the Start Menu entries for Visual Studio (Tools subfolder) which opens a cmd.exe session with the necessary environment variables already set. 
If your Qt installation path has not been added to the global %PATH% variable, you need to execute the "qtenv2.bat" batch file in the Qt folder, e.g. and replace "5.1.1" with the version string of your Qt installation:
```
C:\Qt\Qt5.1.1\5.1.1\msvc2013\bin\qtenv2.bat
```
Go to the EUDAQ-build folder and generate the VS project files:
```
cd c:\[...]\eudaq\build
cmake ..
```
For NI Crate (Mimosa DAQ):
```
cmake -D USE_ROOT=OFF -D BUILD_onlinemon=OFF -D BUILD_ni=ON -D BUILD_tlu=ON ..
```
Installing into eudaq\bin by:
```
cmake --build . --target install --config Release
```
Or by using Visual Studio style:
```
MSBUILD.exe INSTALL.vcxproj /p:Configuration=Release
```

### Notes for OS X

- Compiler: Clang (at least version 3.1)
- Install Qt5 or later, e.g. by using MacPorts (http://www.macports.org/): sudo port install qt5-mac-devel


## Execution

In UNIX:
```
cd ../etc/scripts/
./STARTRUN.example
```
This startrun script starts the Run Control (euRun), the Log Collector (euLog), the Data Collector and the Example Producer:

Play around with Init, Configure, Start, Stop, Re-Start or Re-Configure and Start or Reset and Re-Init and repeat -- or Terminate.

For Initialising and Configuring you have to Load to set the path to the the ini or conf file:
- ../conf/ExampleInit.init
- ../conf/ExampleConfig1.conf

This example stop the first run after 20 events, and re-configure automatically with a 2nd conf file.

A description for operating the EUDET-type beam telescopes can be found here:
https://telescopes.desy.de/User_manual
