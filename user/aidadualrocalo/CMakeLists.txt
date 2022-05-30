get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_DUALROCALO_BUILD "build user/AIDADUALROCALO" OFF)

if(NOT USER_DUALROCALO_BUILD)
    message(STATUS "user/aidadualrocalo is NOT to be built (USER_DUALROCALO_BUILD=OFF)")
  return()
endif()

message(STATUS "user/aidadualrocalo is to be built (USER_DUALROCALO_BUILD=ON)")

add_subdirectory(module)
