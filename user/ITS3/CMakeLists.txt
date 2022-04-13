get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_ITS3_BUILD "build user/ITS3" OFF)

if(NOT USER_ITS3_BUILD)
    message(STATUS "user/ITS3 is NOT to be built (USER_ITS3_BUILD=OFF)")
  return()
endif()

message(STATUS "user/ITS3 is to be built (USER_ITS3_BUILD=ON)")

add_subdirectory(module)
