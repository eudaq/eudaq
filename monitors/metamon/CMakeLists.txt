add_executable(MetaMon.exe   src/MetaMon.cxx  )

target_link_libraries(MetaMon.exe   EUDAQ ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})

INSTALL(TARGETS MetaMon.exe
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
