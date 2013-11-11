set(name "AltroProducer.exe")
set(sourcefiles src/AltroProducer.cc)
set(ext_libraries "" )
set(ext_lib_paths "" )
set(ext_inc_paths $ENV{HOME}/altro_daq/eudaq/readout $ENV{HOME}/altro_daq/eudaq/ilcrorc)

LINK_DIRECTORIES( ${ext_lib_paths} )
INCLUDE_DIRECTORIES( include ${ext_inc_paths})
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
