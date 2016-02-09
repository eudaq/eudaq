set(name "FORTISProducer")
IF(NOT IGNORE_KNOWN_ISSUES AND WIN32) # issues compiling under WINDOWS only
  IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
    MESSAGE(WARNING "There are known issues compiling ${name} on your platform. Set the IGNORE_KNOWN_ISSUES option to configure this producer to be build anyway.")
  ENDIF(NOT BUILD_allproducer)
ELSE(NOT IGNORE_KNOWN_ISSUES AND WIN32)
  FIND_PACKAGE( ROOT REQUIRED)
  INCLUDE_DIRECTORIES( ${ROOT_INCLUDE_DIR} )

  set(sourcefiles src/FORTISPackData.cc src/startExecutableThread.cc)
  set(ext_libraries "" )
  set(ext_lib_paths "" )
  set(ext_inc_paths "" )
  ADD_DEFINITIONS(-DUSE_ROOT=1)

  LINK_DIRECTORIES( ${ext_lib_paths} )
  INCLUDE_DIRECTORIES( include ${ext_inc_paths})
  ADD_EXECUTABLE(${name} src/FORTISProducer.cxx ${sourcefiles})
  ADD_EXECUTABLE(ReadFORTISTimestamps.exe src/ReadFORTISTimestamps.cxx ${sourcefiles})

  TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries} ${ROOT_LIBRARIES})
  TARGET_LINK_LIBRARIES(ReadFORTISTimestamps.exe  EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries}  ${ROOT_LIBRARIES})

  INSTALL(TARGETS ${name} ReadFORTISTimestamps.exe
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF(NOT IGNORE_KNOWN_ISSUES AND WIN32)
