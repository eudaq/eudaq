if(NOT USER_TBSCDESY_EXE)
  message(STATUS "Disable building of aux. TBSC executables user/tbscDESY/exe (USER_AIDASTRIP_EXE=OFF)")
  return()
endif()

set(EXE_CLI_DATACON tbscCliConverter)
add_executable(${EXE_CLI_DATACON} src/tbscCliConverter.cxx)
target_link_libraries(${EXE_CLI_DATACON} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
list(APPEND INSTALL_TARGETS ${EXE_CLI_DATACON})

install(TARGETS ${INSTALL_TARGETS}
   DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib)
