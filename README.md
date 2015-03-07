eudaq [![Build Status](https://travis-ci.org/eudaq/eudaq.svg?branch=v1.4-dev)](https://travis-ci.org/eudaq/eudaq)
=====

A Generic Multi-platform Data Acquisition Framework

### License
This program is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

### First steps

Please check out the online documentation at http://eudaq.github.io/

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
EUDAQ requires a C++11 compliant compiler such as GCC (4.6 and later),
Clang (at least version 3.1), or MSVC (Visual Studio 2012 and later).

### Linux
------------------
- Install Qt4 or later, e.g. by using the package manager of your distribution: ```apt-get install qt4-devel```

### OS X
------------------
- Install Qt4 or later, e.g. by using MacPorts (http://www.macports.org/): ```sudo port install qt4-mac-devel```

### Windows
------------------
- Download and install Qt4 or later
For full description on how to set up the development environment for Windows, see section below.


1.2. Specific Producers and Components
--------------------------------------

### TLU producer
------------------
- (Windows) install libusb (download from http://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/, for documentation see http://sourceforge.net/apps/trac/libusb-win32/wiki) into ./extern/libusb-w32
- (Linux) install libusb development package, e.g. ```apt-get install libusb-dev```
- install ZestSC1 driver package (if AFS is accessible on the machine, this will be installed automatically when running CMake; otherwise, manually copy full folder with sub-directories from /afs/desy.de/group/telescopes/tlu/ZestSC1 to into ./extern subfolder in EUDAQ sources)
- install firmware bitfiles (if AFS is accessible on the machine, this will be installed automatically when runnig CMake; otherwise, manually copy /afs/desy.de/group/telescopes/tlu/tlufirmware into ./extern)

### Online Monitor
--------------------
- requires ROOT (download from http://root.cern.ch/drupal/content/downloading-root)
- (Windows) Make sure that you have installed the corresponding version of MSVC with which your downloaded ROOT binaries have been compiled!


2. Compiling and Installing on Linux/OSX:
------------------------

```
cd build
cmake ..
make install
```

will build main library, executables and gui (if Qt is found) and install into the source directory (./lib and ./bin). The install prefix can be changed by setting ```INSTALL_PREFIX``` option, e.g.:

```
cmake -DINSTALL_PREFIX=/usr/local ..
```

The main library (libEUDAQ.so) is always built, while the rest of the
package is optional. Defaults are to build the main executables and
(if Qt is found) the GUI application. Disable this behavior by setting
e.g. ```BUILD_main=OFF`` (disabling main executables) or enable
e.g. producers using BUILD_tlu=ON to enable setting up the
configuration and compilation environment of tlu producer and
executables.

Example:
```
cd build
cmake -D BUILD_tlu=ON ..
make install
```
Variables thus set are cached in CMakeCache.txt and will again be taken into account at the next cmake run.


2.1. Setting up and Compiling EUDAQ on Windows
-----------------------------------------------------------------------------
- Download Visual Studio Express Desktop (e.g. 2013 Version): http://www.microsoft.com/en-us/download/details.aspx?id=40787
  If you are using MSVC 2010, please make sure that you also install Service Pack 1 (SP1)
- Download Qt4 or Qt5 (use the binaries compatible with your version of MSVC)

Install both packages.

Start the Visual Studio "Developer Command Prompt" from the Start Menu
entries for Visual Studio (Tools subfolder) which opens a cmd.exe
session with the necessary environment variables already set. If your
Qt installation path has not been added to the global %PATH% variable, you need to execute the "qtenv2.bat" batch file in the Qt folder, e.g.
```
C:\Qt\Qt5.1.1\5.1.1\msvc2012\bin\qtenv2.bat
```
Replace "5.1.1" with the version string of your Qt installation.

Now clone the EUDAQ repository (or download using GitHub) and enter the build directory on the prompt, e.g. by entering
```
cd c:\Users\[username]\Documents\GitHub\eudaq\build
```
Now enter

```
cmake ..
```

to generate the VS project files.

Compile by calling

```
MSBUILD.exe EUDAQ.sln /p:Configuration=Release
```

or install into eudaq\bin by running

```
MSBUILD.exe INSTALL.vcxproj /p:Configuration=Release
```

Note on "moc.exe - System Error: The program can't start because MSVCP110.dll is missing from your computer.": when using "Visual Express 2013" and pthreads-w32 2.9.1, you might require "Visual C++ Redistributable for Visual Studio 2012": download (either x86 or x64) from http://www.microsoft.com/en-us/download/details.aspx?id=30679 and install.

This will compile the main library and the GUI; for the remaining processors, please check the individual documentation.

