
cmake_dependent_option(EUDAQ_BUILD_LYCORIS_EXE "build aux. LYCORIS executables." ON
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_BUILD_LYCORIS_EXE)
  message(STATUS "Disable the building of aux. LYCORIS executables (EUDAQ_BUILD_LYCORIS_EXE=OFF)")
  return()
endif()


set(CMAKE_PREFIX_PATH $ENV{ROOTSYS})
set(ROOT_DIR $ENV{ROOTSYS}/cmake)
find_package(ROOT)

set(EXE_CLI_LYCORIS_DATACON lycorisCliConverter)
add_executable(${EXE_CLI_LYCORIS_DATACON} src/lycorisCliConverter.cxx)
target_link_libraries(${EXE_CLI_LYCORIS_DATACON} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES})
list(APPEND INSTALL_TARGETS ${EXE_CLI_LYCORIS_DATACON})

include_directories(${ROOT_INCLUDE_DIR})

install(TARGETS ${INSTALL_TARGETS}
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
