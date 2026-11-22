##-- OPT: Compile FERS?
#option(USER_FERS_BUILD "build FERS" OFF)
#if(NOT USER_FERS_BUILD)
#message(STATUS "user/fers is NOT to be built (USER_FERS_BUILD=OFF)")
# return()
#endif()
#message(STATUS "user/fers is to be built (USER_FERS_BUILD=ON)")

##-- Global Preparation:
#-- EUDAQ_MODULE needed for compilation at lower level CMakeLists
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE eudaq_module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY eudaq_static_${EUDAQ_USERNAME})
set(CAEN_DIGI_LIBRARY CAENDigitizer)
set(CAEN_COMM_LIBRARY CAENComm)
set(CAEN_VME_LIBRARY CAENVME)

include_directories(${EUDAQ_INCLUDE_DIRS})

include_directories(hardware/include)
add_subdirectory(hardware)
##-- Main Producer + DC:
add_subdirectory(module)
