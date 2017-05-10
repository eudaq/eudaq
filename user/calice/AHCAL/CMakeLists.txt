if(WIN32)
  message("user/calice AHCAL is not to be built on windows (fixme)")
  return()
endif()

cmake_dependent_option(USER_CALICE_BUILD_AHCAL "build user/calice AHCAL" ON
  "USER_CALICE_BUILD" OFF)

if(NOT USER_CALICE_BUILD_AHCAL)
  message("user/calice AHCAL is not to be built")
  return()
endif()

set(sourcefiles src/AHCALProducer.cxx src/AHCALProducer.cc src/ScReader.cc)

include_directories(./include)
add_executable(AHCALProducer ${sourcefiles})
target_link_libraries(AHCALProducer ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})

install(TARGETS AHCALProducer
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
