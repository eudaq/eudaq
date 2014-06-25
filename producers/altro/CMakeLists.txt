set(name "AltroProducer.exe")
IF(NOT IGNORE_KNOWN_ISSUES)
  IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
    MESSAGE(WARNING "There are known issues compiling ${name} (see https://github.com/eudaq/eudaq/issues/72). Set the IGNORE_KNOWN_ISSUES option to configure this producer to be build anyway.")
  ENDIF(NOT BUILD_allproducer)
ELSE(NOT IGNORE_KNOWN_ISSUES)
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
ENDIF(NOT IGNORE_KNOWN_ISSUES)