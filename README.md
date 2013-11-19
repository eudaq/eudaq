eudaq
=====

eudaq Data Acquisition Framework main repository

1. Prerequisites
----------------

1.1. Main Library, Executables and GUI
--------------------------------------

1.1.1 Linux
------------------
- Install Qt4 or later

1.1.2 OS X
------------------
- Install Qt4 or later


1.1.3 Windows
------------------
- Install Qt4 or later
- download and install the pthreads library (pre-build binary from ftp://sources.redhat.com/pub/pthreads-win32)
For full description on how to set up the development environment for Windows, see section below.


1.2. Specific Producers and Components
--------------------------------------

1.2.1 TLU producer
------------------
- (Windows) install libusb (download from http://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/, for documentation see http://sourceforge.net/apps/trac/libusb-win32/wiki)
- (Linux) install libusb development package, e.g. 'apt-get install libusb-dev'
- install ZestSC1 driver package (if AFS is accessible on the machine, this will be installed automatically when running CMake; otherwise, manually copy full folder with sub-directories from /afs/desy.de/group/telescopes/tlu/ZestSC1 to into ./extern subfolder in EUDAQ sources)
- install firmware bitfiles (if AFS is accessible on the machine, this will be installed automatically when runnig CMake; otherwise, manually copy /afs/desy.de/group/telescopes/tlu/tlufirmware into ./extern)


2. Compiling and Installing:
------------------------
cd build
cmake ..
make install

will build main library, executables and gui (if Qt is found) and install into the source directory (./lib and ./bin). The install prefix can be changed by setting INSTALL_PREFIX option, e.g.:

cmake -DINSTALL_PREFIX=/usr/local ..


The main library (libEUDAQ.so) is always built, while the rest of the
package is optional. Defaults are to build the main executables and
(if Qt is found) the GUI application. Disable this behavior by setting
e.g. BUILD_main=OFF (disabling main executables) or enable
e.g. producers using BUILD_tlu=ON to enable setting up the
configuration and compilation environment of tlu producer and
executables.

Example:

cd build
cmake -D BUILD_tlu=ON ..
make install

Variables thus set are cached in CMakeCache.txt and will again be taken into account at the next cmake run.


2.1. Setting up Windows Development Environment
-----------------------------------------------
Download Qt4 or Qt5:
Download and install the pthreads library (pre-build binary from ftp://sources.redhat.com/pub/pthreads-win32) into either c:\pthreads-w32 or ./extern/pthreads-w32
Download Visual Studio Express Desktop (e.g. 2013 Version):
(Warning: requires registration)

Install both packages.
Start the Visual Studio "Developer Command Prmopt" from the Start Menu entries vor Visual Studio (Tools subfolder) which opens a cmd.exe session with the necessary environment variables already set. Now execute the "qtenv2.bat" batch file in the Qt folder, e.g.
C:\Qt\Qt5.1.1\5.1.1\msvc2012\bin\qtenv2.bat
Replace "5.1.1" with the version string of your Qt installation.

Now clone the EUDAQ repository (or download using GitHub) and enter the build directory on the prompt, e.g. by entering
cd c:\Users\[username]\Documents\GitHub\eudaq\build

Now enter
cmake ..

to generate the VS project files.

Compile by calling

MSBUILD.exe EUDAQ.sln /p:Configuration=Release


Note: when using "Visual Express 2013" and pthreads-w32 2.9.1, you might require "Visual C++ Redistributable for Visual Studio 2012": download from http://www.microsoft.com/en-us/download/details.aspx?id=30679 and install.


3. Development:
-----------

If you would like to contribute your code back into the main repository, please follow the 'fork & pull request' strategy:

* create a user account on github, log in
* 'fork' the (main) project on github (using the button on the page of the main repo)

* either: clone from the newly forked project and add 'upstream' repository to local clone (change user names in URLs accordingly):

git clone https://github.com/hperrey/eudaq eudaq
cd eudaq
git remote add upstream https://github.com/eudaq/eudaq.git

_OR_ if edits were made to a previous checkout of upstream: rename origin to upstream, add fork as new origin:

cd eudaq
git remote rename origin upstream
git remote add origin https://github.com/hperrey/eudaq
git remote -v show

* optional: edit away on your local clone!

* push the edits to origin (our fork)
git push
(defaults to 'git push origin master' where origin is the repo and master the branch)

* verify that your changes made it to your github fork and then click there on the 'compare & pull request' button

* summarize your changes and click on 'send' 
