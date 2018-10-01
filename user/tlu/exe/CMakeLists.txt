if(NOT EUDAQ_BUILD_EXECUTABLE)
  message(STATUS "Disable the building of main EUDAQ executables (EUDAQ_BUILD_EXECUTABLE=OFF)")
  return()
endif()

set(EXE_CLI_TLU_READER euCliTluReader)
add_executable(${EXE_CLI_TLU_READER} src/euCliTluReader.cxx)
target_link_libraries(${EXE_CLI_TLU_READER} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_TLU_READER})

set(EXE_CLI_TRIGGER_READER euCliTriggerReader)
add_executable(${EXE_CLI_TRIGGER_READER} src/euCliTriggerReader.cxx)
target_link_libraries(${EXE_CLI_TRIGGER_READER} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_TRIGGER_READER})

if(USER_TLU_BUILD_EUDET)
  message(STATUS "Building EUDET TLU stand-alone executables (USER_BUILD_EUDET_TLU=ON)")

  set(EXE_EUDET_TLU_CONTROL EudetTluControl)
  add_executable(${EXE_EUDET_TLU_CONTROL} src/EudetTluControl.cxx)
  target_link_libraries(${EXE_EUDET_TLU_CONTROL} ${EUDAQ_CORE_LIBRARY} ${USER_HARDWARE_LIBRARY})
  list(APPEND INSTALL_TARGETS ${EXE_EUDET_TLU_CONTROL})

  set(EXE_EUDET_TLU_NOROOT EudetTluNoRoot)
  add_executable(${EXE_EUDET_TLU_NOROOT} src/EudetTluNoRoot.cxx)
  target_link_libraries(${EXE_EUDET_TLU_NOROOT} ${EUDAQ_CORE_LIBRARY} ${USER_HARDWARE_LIBRARY})
  list(APPEND INSTALL_TARGETS ${EXE_EUDET_TLU_NOROOT})
  
  set(EXE_EUDET_TLU_RESET EudetTluReset)
  add_executable(${EXE_EUDET_TLU_RESET} src/EudetTluReset.cxx)
  target_link_libraries(${EXE_EUDET_TLU_RESET} ${EUDAQ_CORE_LIBRARY} ${USER_HARDWARE_LIBRARY})
  list(APPEND INSTALL_TARGETS ${EXE_EUDET_TLU_RESET})
else()
  message(STATUS "EUDET TLU stand-alone executables are NOT to be built (USER_BUILD_EUDET_TLU=OFF)")
endif()

install(TARGETS ${INSTALL_TARGETS}
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
