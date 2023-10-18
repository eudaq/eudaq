if(WIN32)
  message(STATUS "PIStage producer is not supported on Windows, you are very
  welcome to implement the windows functionalitites")
  return()
endif()

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)

set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_PISTAGE_BUILD "build user/piStage" OFF)

if(NOT USER_PISTAGE_BUILD)
    message(STATUS "user/piStage is NOT to be built (USER_PISTAGE_BUILD=OFF)")
  return()
endif()
message(STATUS "user/piStage is to be built (USER_PISTAGE_BUILD=ON)")

include_directories(${EUDAQ_INCLUDE_DIRS})



include_directories(hardware/include)
add_subdirectory(hardware)
add_subdirectory(module)
