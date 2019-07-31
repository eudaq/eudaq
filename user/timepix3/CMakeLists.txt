list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_TIMEPIX3_BUILD "build timepix3" OFF)

if(NOT USER_TIMEPIX3_BUILD)
  message(STATUS "user/timepix3 is NOT to be built (USER_TIMEPIX3_BUILD=OFF)")
  return()
else()
  message(STATUS "user/timepix3 is to be built (USER_TIMEPIX3_BUILD=ON)")
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})

# Packages required for the producer. If not present, only the data converter is built
FIND_PACKAGE(SPIDR)
FIND_PACKAGE(XercesC)

add_subdirectory(module)
