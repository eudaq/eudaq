add_executable(NiProducer src/NiProducer.cxx)
target_link_libraries(NiProducer ${EUDAQ_CORE_LIBRARY})
list(APPEND INSTALL_TARGETS NiProducer)

add_executable(TelTest src/TelTest.cxx)
target_link_libraries(TelTest ${EUDAQ_CORE_LIBRARY})
list(APPEND INSTALL_TARGETS TelTest)
  
install(TARGETS
  ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
