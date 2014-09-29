set(name "DEPFETProducerTCP.exe")
IF(NOT IGNORE_KNOWN_ISSUES AND WIN32) # issues compiling under WINDOWS only
  IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
    MESSAGE(WARNING "There are known issues compiling ${name} on your platform. Set the IGNORE_KNOWN_ISSUES option to configure this producer to be build anyway.")
  ENDIF(NOT BUILD_allproducer)
ELSE(NOT IGNORE_KNOWN_ISSUES AND WIN32)

  # Add Converter plugin to the list of plugins to be baked into the main EUDAQ library:
  add_plugin_sources(src/DEPFETConverterPlugin.cc)

  set(sourcefiles src/DEPFETProducerTCP.cxx src/TCPclient.cc src/rc_depfet.cc)
  set(ext_libraries "" )
  set(ext_lib_paths "" )
  set(ext_inc_paths "" )

  LINK_DIRECTORIES( ${ext_lib_paths} )
  INCLUDE_DIRECTORIES( include ${ext_inc_paths})
  ADD_EXECUTABLE(${name} ${sourcefiles})

  TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})

  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF(NOT IGNORE_KNOWN_ISSUES AND WIN32)