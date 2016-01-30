set(name "NiProducer")
set(sourcefiles src/NiProducer.cxx src/NiController.cc)

INCLUDE_DIRECTORIES( include )
add_executable(${name} ${sourcefiles})

target_link_libraries(${name}   EUDAQ ${EUDAQ_THREADS_LIB})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
