list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})

option(USER_CMSIT_BUILD "build CMSIT" OFF)

if(NOT USER_CMSIT_BUILD)
  message(STATUS "user/CMSIT is NOT to be built (USER_CMSIT_BUILD=OFF)")
  return()
else()
  message(STATUS "user/CMSIT is to be built (USER_CMSIT_BUILD=ON)")
endif()


add_subdirectory(module)
