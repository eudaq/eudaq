set(name "AltroUSBProducer.exe")
IF(NOT IGNORE_KNOWN_ISSUES)
  IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
    MESSAGE(WARNING "There are known issues compiling ${name} (see https://github.com/eudaq/eudaq/issues/73). Set the IGNORE_KNOWN_ISSUES option to configure this producer to be build anyway.")
  ENDIF(NOT BUILD_allproducer)
ELSE(NOT IGNORE_KNOWN_ISSUES)

  # Add Converter plugin to the list of plugins to be baked into the main EUDAQ library:
  add_plugin_sources(src/AltroUSBConverterPlugin.cc)

  set(sourcefiles src/AltroUSBProducer.cxx src/AltroUSBProducer.cc)
  set(ext_libraries ilcdaq_usb )
  set(ext_lib_paths /usr/local/lib/altro)

  LINK_DIRECTORIES( ${ext_lib_paths} )
  INCLUDE_DIRECTORIES( include )
  ADD_EXECUTABLE(${name} ${sourcefiles})

  TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})

  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF(NOT IGNORE_KNOWN_ISSUES)