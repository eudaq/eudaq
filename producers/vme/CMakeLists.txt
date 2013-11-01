set(sourcefiles src/OldVMEInterface.cc src/TSI148Interface.cc src/VMEInterface.cc src/vmelib.c)
set(ext_libraries "vme" )
set(ext_lib_paths "" )
set(ext_inc_paths "../../extern/Tsi148" )

ADD_DEFINITIONS(VME_TRACE=0)
LINK_DIRECTORIES( ${ext_lib_paths} )
INCLUDE_DIRECTORIES( include ${ext_inc_paths})

ADD_EXECUTABLE(VMEReset.exe src/VMEReset.cxx ${sourcefiles})
ADD_EXECUTABLE(VMETest.exe  src/VMETest.cxx ${sourcefiles})
ADD_EXECUTABLE(Scaler.exe   src/Scaler.cxx ${sourcefiles})

TARGET_LINK_LIBRARIES(VMEReset.exe   EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${ext_libraries})
TARGET_LINK_LIBRARIES(VMETest.exe    EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${ext_libraries})
TARGET_LINK_LIBRARIES(Scaler.exe     EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${ext_libraries})

INSTALL(TARGETS VMEReset.exe VMETest.exe Scaler.exe
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
