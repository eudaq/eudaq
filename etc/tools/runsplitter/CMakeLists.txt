SET(name "EURunsplitter.exe")

SET(sourcefiles src/Runsplitter.cc)

ADD_EXECUTABLE(${name} ${sourcefiles})

target_link_libraries(${name} EUDAQ ${EUDAQ_THREADS_LIB})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
