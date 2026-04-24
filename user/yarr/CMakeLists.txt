get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})

option(USER_YARR_BUILD "build user/yarr" OFF)

if(NOT USER_YARR_BUILD)
    message(STATUS "user/yarr is NOT to be built (USER_YARR_BUILD=OFF)")
  return()
endif()

message(STATUS "user/yarr is to be built (USER_YARR_BUILD=ON)")

add_subdirectory(module)
