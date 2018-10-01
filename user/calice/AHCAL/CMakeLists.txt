# if(WIN32)
#   message("user/calice AHCAL is not to be built on windows (fixme)")
#   return()
# endif()

cmake_dependent_option(USER_CALICE_BUILD_AHCAL "build user/calice AHCAL" ON
  "USER_CALICE_BUILD" OFF)

get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME}) 

if(NOT USER_CALICE_BUILD_AHCAL)
  message("user/calice AHCAL is not to be built")
  return()
endif()

add_subdirectory(module)

# set(sourcefiles src/AHCALProducer.cxx src/AHCALProducer.cc src/ScReader.cc)

# include_directories(./include)
# add_executable(AHCALProducer ${sourcefiles})
# target_link_libraries(AHCALProducer ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})

# install(TARGETS AHCALProducer
#  RUNTIME DESTINATION bin
#  LIBRARY DESTINATION lib
#  ARCHIVE DESTINATION lib)
