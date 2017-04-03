option(EUDAQ_BUILD_EXECUTABLE "Compile main EUDAQ executables?" ON)
if(NOT EUDAQ_BUILD_EXECUTABLE)
  message(STATUS "Disable the building of main EUDAQ executables (EUDAQ_BUILD_EXECUTABLE=OFF)")
  return()
endif()

add_executable(Launcher src/Launcher.cxx)
target_link_libraries(Launcher ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS Launcher)

set(EXE_CLI_PRODUCER euCliProducer)
add_executable(${EXE_CLI_PRODUCER} src/euCliProducer.cxx)
target_link_libraries(${EXE_CLI_PRODUCER} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_PRODUCER})

set(EXE_CLI_DATACOL euCliDataCollector)
add_executable(${EXE_CLI_DATACOL} src/euCliDataCollector.cxx)
target_link_libraries(${EXE_CLI_DATACOL} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_DATACOL})

set(EXE_CLI_RUN euCliRun)
add_executable(${EXE_CLI_RUN} src/euCliRun.cxx)
target_link_libraries(${EXE_CLI_RUN} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_RUN})

set(EXE_CLI_LOG euCliLog)
add_executable(${EXE_CLI_LOG} src/euCliLog.cxx)
target_link_libraries(${EXE_CLI_LOG} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_LOG})

set(EXE_CLI_MON euCliMonitor)
add_executable(${EXE_CLI_MON} src/euCliMonitor.cxx)
target_link_libraries(${EXE_CLI_MON} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_MON})

install(TARGETS ${INSTALL_TARGETS}
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
