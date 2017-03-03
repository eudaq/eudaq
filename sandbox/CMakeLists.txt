SET(name "RpiTestProducer.exe")
SET(sourcefiles src/RpiTestProducer.cxx)

ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name} EUDAQ ${EUDAQ_THREADS_LIB})

INSTALL(TARGETS  ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
