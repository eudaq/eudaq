get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})

option(USER_XROOTD_BUILD "build user/xrootd" OFF)

if(NOT USER_XROOTD_BUILD)
    message(STATUS "user/xrootd is NOT to be built (USER_XROOTD_BUILD=OFF)")
  return()
endif()

message(STATUS "user/xrootd is to be built (USER_XROOTD_BUILD=ON)")

add_subdirectory(module)
