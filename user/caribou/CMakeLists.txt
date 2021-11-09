get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

# DSO9254A converter needs root
find_package(ROOT)
cmake_dependent_option(USER_CARIBOU_BUILD "DSO9254A converter needs ROOT" ON
  "ROOT_FOUND" OFF)

option(USER_CARIBOU_BUILD "build caribou support" OFF)

if(NOT USER_CARIBOU_BUILD)
  message(STATUS "user/caribou is NOT to be built (USER_CARIBOU_BUILD=OFF)")
  return()
else()
  message(STATUS "user/caribou is to be built (USER_CARIBOU_BUILD=ON)")
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})

FIND_PACKAGE(Peary REQUIRED)

add_subdirectory(module)
