include_directories(${ROOT_INCLUDE_DIR})
if(ROOT_USE_FILE)
  include(${ROOT_USE_FILE})
endif()

include_directories(. include)
include_directories(${CMAKE_BINARY_DIR})

SET(ROOTProducer_name ROOTProducer)
SET(ROOTProducer_SOURCES
  src/ROOTProducer.cc)

ROOT_GENERATE_DICTIONARY(${ROOTProducer_name}_ROOT  include/ROOTProducer.hh)
ADD_LIBRARY(${ROOTProducer_name} SHARED ${ROOTProducer_SOURCES} ${ROOTProducer_name}_ROOT.cxx)
target_link_libraries(${ROOTProducer_name} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES})


INSTALL(TARGETS ${ROOTProducer_name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  )
