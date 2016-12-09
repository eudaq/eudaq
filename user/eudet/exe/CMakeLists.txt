add_executable(NiProducer src/NiProducer.cxx)
target_link_libraries(NiProducer ${EUDAQ_CORE})
list(APPEND INSTALL_TARGETS NiProducer)

if(USER_TELESCOPE_HARDWARE)
  add_executable(TLUProducer src/TLUProducer.cxx)
  add_executable(TLUControl src/TLUControl.cxx)
  add_executable(TLUReset src/TLUReset.cxx)
  target_link_libraries(TLUProducer ${EUDAQ_CORE} ${USER_HARDWARE})
  target_link_libraries(TLUControl ${EUDAQ_CORE} ${USER_HARDWARE})
  target_link_libraries(TLUReset ${EUDAQ_CORE} ${USER_HARDWARE})
  list(APPEND INSTALL_TARGETS TLUProducer TLUControl TLUReset)
  
  add_executable(TelTest src/TelTest.cxx)
  target_link_libraries(TelTest ${EUDAQ_CORE} ${USER_HARDWARE})
  list(APPEND INSTALL_TARGETS TelTest)
endif()
  
install(TARGETS
  ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
