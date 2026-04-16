##-- OPT: Compile FERS?
option(USER_HIDRA_BUILD "Build HiDRa DAQ system" OFF)
if(NOT USER_HIDRA_BUILD)
    message(STATUS "user/hidra is NOT to be built (USER_HIDRA_BUILD=OFF)")
  return()
endif()
message(STATUS "user/hidra is to be built (USER_HIDRA_BUILD=ON)")

##-- Global Preparation:
#-- EUDAQ_MODULE needed for compilation at lower level CMakeLists
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE eudaq_module_${EUDAQ_USERNAME})

include_directories(${EUDAQ_INCLUDE_DIRS})

##-- DAQ Components
#-- Add component folder
add_subdirectory(fers) 
add_subdirectory(xdc)
