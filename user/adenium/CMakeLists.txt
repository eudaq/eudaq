list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

##-- OPT: Compile ADENIUM?
option(USER_ADENIUM_BUILD "build ADENIUM" ON)
if(NOT USER_ADENIUM_BUILD)
    message(STATUS "user/adenium is NOT to be built (USER_ADENIUM_BUILD=OFF)")
  return()
endif()
message(STATUS "user/adenium is to be built (USER_ADENIUM_BUILD=ON)")


##-- converter plugin
add_subdirectory(module)

