if(NOT EUDAQ_BUILD_EXECUTABLE)
  message(STATUS "Disable the building of main EUDAQ executables (EUDAQ_BUILD_EXECUTABLE=OFF)")
  return()
endif()

set(EXE_CLI_TELE_READER euCliTeleReader)
add_executable(${EXE_CLI_TELE_READER} src/euCliTeleReader.cxx)
target_link_libraries(${EXE_CLI_TELE_READER} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_TELE_READER})

install(TARGETS ${INSTALL_TARGETS}
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

  
