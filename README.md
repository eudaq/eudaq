eudaq 1
=====

[![Build Status](https://travis-ci.org/eudaq/eudaq.svg?branch=v1.7-dev)](https://travis-ci.org/eudaq/eudaq) 
[![Build status](https://ci.appveyor.com/api/projects/status/n3tq45kkupyvjihg/branch/v1.6-dev?svg=true)](https://ci.appveyor.com/project/eudaq/eudaq/branch/v1.7-dev)

A Generic Multi-platform Data Acquisition Framework

### License
This program is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

### First steps

Please check out the online documentation at 
- http://eudaq.github.io/ or 
- https://telescopes.desy.de/EUDAQ or
- the manual, see below

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

1. Prerequisites for installation
---------------------------------

1.1. Main Library, Executables and GUI
--------------------------------------
EUDAQ requires a C++11 compliant compiler and a Qt version 4 or higher to build GUIs.

### Linux
------------------
#### gcc
Version (```gcc --version```): It is observed using older versions of gcc (4.6.3 e.g), it is not working. We recommend version 4.9 or later:
``` 
sudo apt-get install gcc-4.9-multilib
```
Setting environment variables:
``` 
export CXX=/usr/bin/g++-4.9
export CC=/usr/bin/gcc-4.9
```
Setting gcc version (symlink):
http://askubuntu.com/questions/26498/choose-gcc-and-g-version


#### Qt 
Install Qt4 or later, e.g. by using the package manager of your distribution: ```apt-get install qt4-devel```
(In Ubuntu 12: ```sudo apt-get install qt-sdk``` and ```apt-cache search qtcreator```)




### Windows
------------------
#### MSVC
- The recommende windows compiler is MSVC (Microsoft Visual C++) like Visual Studio 2012 and later
- Download Visual Studio Express Desktop (e.g. 2013 Version): http://www.microsoft.com/en-us/download/details.aspx?id=40787
- If you are using MSVC 2010, please make sure that you also install Service Pack 1 (SP1)
#### QT
- Download and install Qt4 or later
- Use the binaries compatible with your version of MSVC
- For full description on how to set up the development environment for Windows, see section below (2. WIndows).


### OS X
------------------
#### Clang
Clang (at least version 3.1)
#### Qt
Install Qt4 or later, e.g. by using MacPorts (http://www.macports.org/): ```sudo port install qt4-mac-devel```


1.2. Specific Producers and Components
--------------------------------------

### TLU producer
------------------
- install ZestSC1 driver package (if AFS is accessible on the machine, this will be installed automatically when running CMake; otherwise, manually copy full folder with sub-directories from /afs/desy.de/group/telescopes/tlu/ZestSC1 to into ./extern subfolder in EUDAQ sources)
- install firmware bitfiles (if AFS is accessible on the machine, this will be installed automatically when runnig CMake; otherwise, manually copy /afs/desy.de/group/telescopes/tlu/tlufirmware into ./extern)

#### Windows
install libusb (download from http://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/, for documentation see http://sourceforge.net/apps/trac/libusb-win32/wiki) into ./extern/libusb-w32
#### Linux
install libusb development package, e.g. ```apt-get install libusb-dev```

### Online Monitor
--------------------
- requires ROOT (download from http://root.cern.ch/drupal/content/downloading-root) with the `-lPhysics` library compiled
- (Windows) Make sure that you have installed the corresponding version of MSVC with which your downloaded ROOT binaries have been compiled!


2. Configuring and installation/compiling EUDAQ
----------------------------------

cmake will configure the installation and prepare the makefiles. It searches for all the required files. It has to be executed in the ```build``` folder, however the relevant CMakeLists.txt is in the main level, thus, the command is ```cmake ..```. If cmake is successful, EUDAQ can be installed. Variables set are cached in CMakeCache.txt and will again be taken into account at the next cmake run.

cmake has several options to activate or deactivate programs which will be built. The main library (libEUDAQ.so) is always built, while the rest of the package is optional. Defaults are to build the main executables and
(if Qt is found) the GUI application. Disable this behavior by setting
e.g. ```-DBUILD_main=OFF``` (disabling main executables) or enable
e.g. producers using ```-DBUILD_tlu=ON``` to enable setting up the
configuration and compilation environment of tlu producer and
executables. Example: ```cmake -DBUILD_tlu=ON ..```. More options:
https://telescopes.desy.de/EUDAQ#Cmake_options

If cmake is not successful and complains about something is missing, it is recommended to clean the ```build``` folder, before a new try, e.g. .

If problems occur during installation, please have a look in the issues, if a similiar problem already occured. If not, feel free to create a new ticket: https://github.com/eudaq/eudaq/issues

### Linux/OSX:

Configuring:
```
cd build
cmake ..
```
Installing:
```
make install
```
Cleaning:
```
rm -rf *
```

### Windows

Start the Visual Studio "Developer Command Prompt" from the Start Menu entries for Visual Studio (Tools subfolder) which opens a ```cmd.exe``` session with the necessary environment variables already set. If your Qt installation path has not been added to the global %PATH% variable, you need to execute the "qtenv2.bat" batch file in the Qt folder, e.g.
```
C:\Qt\Qt5.1.1\5.1.1\msvc2012\bin\qtenv2.bat
```
(Replace "5.1.1" with the version string of your Qt installation.)

Go to the EUDAQ folder and configure:
```
cd c:\[...]\eudaq\build
cmake ..
```
(This generates the VS project files.)

Installing into eudaq\bin:
```
MSBUILD.exe INSTALL.vcxproj /p:Configuration=Release
```
(Alternative: ```MSBUILD.exe EUDAQ.sln /p:Configuration=Release```)

(Note on "moc.exe - System Error: The program can't start because MSVCP110.dll is missing from your computer.": when using "Visual Express 2013" and pthreads-w32 2.9.1, you might require "Visual C++ Redistributable for Visual Studio 2012": download (either x86 or x64) from http://www.microsoft.com/en-us/download/details.aspx?id=30679 and install.)

This will compile the main library and the GUI; for the remaining processors, please check the individual documentation.
