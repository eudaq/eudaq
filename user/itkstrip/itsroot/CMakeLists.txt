include_directories(. include)
include_directories(${CMAKE_BINARY_DIR})

SET(ROOTProducer_name ${EUDAQ_PREFIX}ROOTProducer)
SET(ROOTProducer_SOURCES
  src/ROOTProducer.cc)

ROOT_GENERATE_DICTIONARY(${ROOTProducer_name}_ROOT  include/ROOTProducer.hh MODULE ${ROOTProducer_name})
ADD_LIBRARY(${ROOTProducer_name} SHARED ${ROOTProducer_SOURCES} )
target_link_libraries(${ROOTProducer_name} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ROOT::Core)


INSTALL(TARGETS ${ROOTProducer_name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  )
