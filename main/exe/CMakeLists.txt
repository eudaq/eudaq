option(EUDAQ_BUILD_EXECUTABLE "Compile main EUDAQ executables?" ON)
if(NOT EUDAQ_BUILD_EXECUTABLE)
  message(STATUS "Disable the building of main EUDAQ executables (EUDAQ_BUILD_EXECUTABLE=OFF)")
  return()
endif()

set(EXE_CLI_PRODUCER euCliProducer)
add_executable(${EXE_CLI_PRODUCER} src/euCliProducer.cxx)
target_link_libraries(${EXE_CLI_PRODUCER} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_PRODUCER})

set(EXE_CLI_DATACOL euCliCollector)
add_executable(${EXE_CLI_DATACOL} src/euCliDataCollector.cxx)
target_link_libraries(${EXE_CLI_DATACOL} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_DATACOL})

set(EXE_CLI_DATACOL euCliConverter)
add_executable(${EXE_CLI_DATACOL} src/euCliConverter.cxx)
target_link_libraries(${EXE_CLI_DATACOL} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_DATACOL})

set(EXE_CLI_RUN euCliRun)
add_executable(${EXE_CLI_RUN} src/euCliRun.cxx)
target_link_libraries(${EXE_CLI_RUN} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_RUN})

set(EXE_CLI_LOG euCliLogger)
add_executable(${EXE_CLI_LOG} src/euCliLog.cxx)
target_link_libraries(${EXE_CLI_LOG} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_LOG})

set(EXE_CLI_MON euCliMonitor)
add_executable(${EXE_CLI_MON} src/euCliMonitor.cxx)
target_link_libraries(${EXE_CLI_MON} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_MON})

set(EXE_CLI_HASH euCliHash)
add_executable(${EXE_CLI_HASH} src/euCliHash.cxx)
target_link_libraries(${EXE_CLI_HASH} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_HASH})

set(EXE_CLI_READER euCliReader)
add_executable(${EXE_CLI_READER} src/euCliReader.cxx)
target_link_libraries(${EXE_CLI_READER} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_READER})

install(TARGETS ${INSTALL_TARGETS}
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


#https://stackoverflow.com/questions/59376924/ctest-cmake-mingw-executables-build-but-fail-to-run-because-fresh-dll-is-n
enable_testing()
add_test(
   NAME test_mimosa_tlu_io
   COMMAND euCliReader -i "${CMAKE_SOURCE_DIR}/testing/data/mimosa_tlu.raw" -std -e 0 -E 5 -s
)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
set_tests_properties(test_mimosa_tlu_io
   PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<JOIN:$<TARGET_RUNTIME_DLL_DIRS:euCliReader>,\;>")
endif()
