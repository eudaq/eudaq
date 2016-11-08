
if(USER_TELESCOPE_SHARED)
add_executable(TLUProducer src/TLUProducer.cxx)
add_executable(TLUControl src/TLUControl.cxx)
add_executable(TLUReset src/TLUReset.cxx)
add_executable(NiProducer src/NiProducer.cxx)

target_link_libraries(TLUProducer ${EUDAQ_CORE} ${USER_SHARED})
target_link_libraries(TLUControl ${EUDAQ_CORE} ${USER_SHARED})
target_link_libraries(TLUReset ${EUDAQ_CORE} ${USER_SHARED})
target_link_libraries(NiProducer ${EUDAQ_CORE} ${USER_SHARED})

add_executable(TelTest src/TelTest.cxx)
target_link_libraries(TelTest ${EUDAQ_CORE} ${USER_SHARED})
install(TARGETS
  TelTest
  RUNTIME DESTINATION bin)

install(TARGETS
  TLUProducer TLUControl TLUReset NiProducer
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  )

endif()
