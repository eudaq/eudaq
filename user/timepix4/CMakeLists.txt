list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_TIMEPIX4_BUILD "build timepix4" OFF)

if(NOT USER_TIMEPIX4_BUILD)
  message(STATUS "user/timepix4 is NOT to be built (USER_TIMEPIX4_BUILD=OFF)")
  return()
else()
  message(STATUS "user/timepix4 is to be built (USER_TIMEPIX4_BUILD=ON)")
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})

# Packages required for the producer. If not present, only the data converter is built
FIND_PACKAGE(SPIDR4)

add_subdirectory(module)
