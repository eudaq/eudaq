get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_CARIBOU_BUILD "build caribou support" OFF)

if(NOT USER_CARIBOU_BUILD)
  message(STATUS "user/caribou is NOT to be built (USER_CARIBOU_BUILD=OFF)")
  return()
else()
  message(STATUS "user/caribou is to be built (USER_CARIBOU_BUILD=ON)")
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})

FIND_PACKAGE(Peary 0.15.2 REQUIRED)

add_subdirectory(module)
