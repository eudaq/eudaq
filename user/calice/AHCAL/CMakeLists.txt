cmake_dependent_option(USER_CALICE_AHCAL "build user/calice AHCAL" ON
  "USER_CALICE" OFF)

if(NOT USER_CALICE_AHCAL)
  message("user/calice AHCAL is not to be built")
  return()
endif()

set(sourcefiles src/AHCALProducer.cxx src/AHCALProducer.cc src/ScReader.cc)

include_directories(./include)
add_executable(AHCALProducer ${sourcefiles})
target_link_libraries(AHCALProducer EUDAQ ${EUDAQ_THREADS_LIB})

install(TARGETS AHCALProducer
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
