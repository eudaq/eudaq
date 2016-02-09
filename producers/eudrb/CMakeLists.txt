set(name "EUDRBProducer")
IF(NOT IGNORE_KNOWN_ISSUES AND WIN32) # issues compiling under WINDOWS only
  IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
    MESSAGE(WARNING "There are known issues compiling ${name} on your platform. Set the IGNORE_KNOWN_ISSUES option to configure this producer to be build anyway.")
  ENDIF(NOT BUILD_allproducer)
ELSE(NOT IGNORE_KNOWN_ISSUES AND WIN32)
  set(sourcefiles src/EUDRBProducer.cxx src/EUDRBController.cc)
  set(incdirs vmelib/include )

  # include and build vmelib
  add_subdirectory( vmelib )

  ADD_DEFINITIONS("-DVME_TRACE=0")
  INCLUDE_DIRECTORIES( include ${incdirs})
  ADD_EXECUTABLE(${name} ${sourcefiles})

  TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} vmelib)

  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF(NOT IGNORE_KNOWN_ISSUES AND WIN32)
