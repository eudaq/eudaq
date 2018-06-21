

# AIDA2020 Strip Telescope
## implementation with EUDAQ2

### Compile with a customized Qt5

The KPiX DAQ was based on an old Qt4 version with a lot of dependencies, that are not compatible with one Qt5 installed under a normal system directory.

There are 2 ways to do it, assuming your Qt5 is installed under '/opt/Qt5.9.3' and your Qt5Widgets is '/opt/Qt5.9.3/lib/cmake/Qt5Widgets/'.
1. with 'cmake-gui ..': when you start 'configure', you will see 'Qt5Widgets_DIR NOT FOUND' in an 'ungrouped terms' section, you click the '...' then you can choose where you installed the Qt5;
2. with 'cmake':
'''
cmake -DQt5Widgets_DIR=/opt/Qt5.9.3/lib/cmake/Qt5Widgets/ ...
'''
If you need to switch off some defaul on options, do like "-DUSER_TBSCDESY=OFF" to switch off the tbsc user module.

### Pre-request

You will need the minimum kpix DAQ installed with shared libraries and headfiles provided.


-------------------------
ChangLog

15-June-2017 Mengqing WU

M: Changes in the ./module/CMakelists.txt.
   One needs inherited class headfiles to compile, then to link to the .so shared library files.
   --> in order to include the headfiles from kpix daq to compile:
   "include_directories($DIR)" with $DIR set to the headfile directories.
   --> in order to link to the kpix shared lib (*libkpix.so*), one needs to specify the lib directory. (adding the dir via -L option in target_link_libraries() function)
   *libkpix.so: the shared lib file prepared in advance (compiled in kpix framework)
   *${kpix-Clevel}.h files: each kpix c-level file dir is currently added using include_directories() function (to be updated to a better way).
   
