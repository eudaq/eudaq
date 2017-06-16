ChangLog

15-June-2017 Mengqing WU

M: Changes in the ./module/CMakelists.txt.
   One needs inherited class headfiles to compile, then to link to the .so shared library files.
   --> in order to include the headfiles from kpix daq to compile:
   "include_directories($DIR)" with $DIR set to the headfile directories.
   --> in order to link to the kpix shared lib (*libkpix.so*), one needs to specify the lib directory. (adding the dir via -L option in target_link_libraries() function)
   *libkpix.so: the shared lib file prepared in advance (compiled in kpix framework)
   *${kpix-Clevel}.h files: each kpix c-level file dir is currently added using include_directories() function (to be updated to a better way).
   
