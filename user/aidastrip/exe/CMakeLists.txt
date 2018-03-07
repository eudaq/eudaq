if(NOT USER_AIDASTRIP_EXE)
  message(STATUS "Disable the building of aux. AIDASTRIP executables (USER_AIDASTRIP_EXE=OFF)")
  return()
endif()

set(CMAKE_PREFIX_PATH $ENV{ROOTSYS})
set(ROOT_DIR $ENV{ROOTSYS}/cmake)

set(EXE_CLI_LYCORIS_DATACON lycorisCliConverter)
add_executable(${EXE_CLI_LYCORIS_DATACON} src/lycorisCliConverter.cxx)
target_link_libraries(${EXE_CLI_LYCORIS_DATACON} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES}
  -L/usr/local/lib/kpix/ -lkpix )
list(APPEND INSTALL_TARGETS ${EXE_CLI_LYCORIS_DATACON})

include_directories(. include)
include_directories(${ROOT_INCLUDE_DIR})
include_directories(/usr/local/include/kpix)

##--- set run path of the dynamic kpix lib
set_target_properties(${INSTALL_TARGETS} PROPERTIES INSTALL_RPATH
  ${EUDAQ_INSTALL_RPATH}:/usr/local/lib/kpix/)

install(TARGETS ${INSTALL_TARGETS}
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
