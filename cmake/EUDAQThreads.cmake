# this script checks that we have threading support available;
# either via C++11 or platform-dependent libraries (i.e. phtreads variants).
# sets include directories and definitions accordingly;
# linker library flags will be provided through ${EUDAQ_THREADS_LIB} variable.


# C++11 compliance had been checked before in main CMakeLists.txt file
# and would be sufficient to provide threading
if (USE_CXX11 AND CXX11_COMPILER)
  # nothing to be done here
  message(STATUS "Use C++11 for threading interface")
  Find_Package(Threads REQUIRED)
  set(EUDAQ_THREADS_LIB ${CMAKE_THREAD_LIBS_INIT})

elseif(WIN32) # Windows specific checks
  message(STATUS "Looking for pthreads-w32 library")
  Find_Package(W32Pthread REQUIRED)
  set(EUDAQ_THREADS_LIB ${W32PTHREADS_LIBRARIES})
  INCLUDE_DIRECTORIES( ${W32PTHREADS_INCLUDE_DIRS} )
  # also install the pthread library into the bin directory so it's found by the .exe
  STRING(REGEX REPLACE "/lib/" "/dll/" 
                     DLL_PATH ${W32PTHREADS_LIBRARIES})
  STRING(REGEX REPLACE ".lib" ".dll" 
                     DLL_PATH ${DLL_PATH})
  message(STATUS "found library in ${DLL_PATH}")
  INSTALL(FILES ${DLL_PATH} DESTINATION bin)

else() # unix-based platform (Darwin/Linux)
  message(STATUS "Looking for POSIX threading library")
  # use the threading finding scripts provided through CMake
  Find_Package(Threads REQUIRED)
  set(EUDAQ_THREADS_LIB ${CMAKE_THREAD_LIBS_INIT})

endif(USE_CXX11 AND CXX11_COMPILER)