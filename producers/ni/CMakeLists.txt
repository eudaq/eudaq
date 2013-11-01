set(name "NiProducer.exe")
set(sourcefiles src/NiProducer.cxx src/NiController.cc)

INCLUDE_DIRECTORIES( include )
add_executable(${name} ${sourcefiles})

target_link_libraries(${name}   EUDAQ ${CMAKE_THREAD_LIBS_INIT})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
