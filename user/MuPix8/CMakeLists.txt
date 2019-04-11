get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_MuPix8_BUILD "build user/MuPix8" OFF)

if(NOT USER_MuPix8_BUILD)
    message(STATUS "user/MuPix8 is NOT to be built (USER_MuPix8_BUILD=OFF)")
  return()
endif()

message(STATUS "user/MuPix8 is to be built (USER_MuPix8_BUILD=ON)")

add_subdirectory(module)
