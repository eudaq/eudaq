Development-note
=====


### 16-06-2017 MengqingWu

!important: qt version is now required to use qt5 instead of qt4! check the .travis/xxx.sh file if no idea how to install. => this may end up with no error message printed when cmake-make, but no valid euRun produced at the end.

!Error report:
--> you may meet the following output:

[ 73%] Building CXX object user/aidastrip/module/CMakeFiles/module_aidastrip.dir/src/kpixProducer.cc.o
[ 79%] Built target module_gui
[ 87%] Built target euLog
[ 97%] Built target StdEventMonitor
[ 98%] Linking CXX shared library libeudaq_module_aidastrip.so
CMakeFiles/module_aidastrip.dir/src/kpixProducer.cc.o: In function `kpixProducer::DoInitialise()':
/home/kpix/afs/eudaq/eudaq_dev/user/aidastrip/module/src/kpixProducer.cc:73: undefined reference to `KpixControl::KpixControl(CommLink*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, unsigned int)'
collect2: error: ld returned 1 exit status

Then it may come from the fact that the libkpix.so was produced in a different cxx environment.