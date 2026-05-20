get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})

option(USER_SOURCEMETER_BUILD "build user/sourcemeter" OFF)

if(NOT USER_SOURCEMETER_BUILD)
    message(STATUS "user/sourcemeter is NOT to be built (USER_SOURCEMETER_BUILD=OFF)")
  return()
endif()

message(STATUS "user/sourcemeter is to be built (USER_SOURCEMETER_BUILD=ON)")

if(NOT EUDAQ_BUILD_PYTHON)
  message(WARNING "Sourcemeter producer requires the Python EUDAQ binding library, but EUDAQ_BUILD_PYTHON is OFF")
endif()

