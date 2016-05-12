set(name "Mimosa26_SBG_Producer.exe")

FIND_PACKAGE(IPHCRC REQUIRED)

IF(IPHCRC_FOUND)
ELSE(IPHCRC_FOUND)
  MESSAGE(FATAL_ERROR "Could not find IPHCRC lib or header. check line above for details!")
ENDIF(IPHCRC_FOUND)

set(sourcefiles src/M26Producer.cxx src/M26Controller.cc)

INCLUDE_DIRECTORIES( include ${IPHCRC_INCLUDE_DIRS})
add_executable(${name} ${sourcefiles})

target_link_libraries(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${IPHCRC_LIBRARIES})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
