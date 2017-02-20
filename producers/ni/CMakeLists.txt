set(name "NiProducer.exe")
set(sourcefiles src/NiProducer.cxx src/NiController.cc)

INCLUDE_DIRECTORIES( include )
add_executable(${name} ${sourcefiles})

target_link_libraries(${name}   EUDAQ )

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin COMPONENT NI
  LIBRARY DESTINATION lib COMPONENT NI
  ARCHIVE DESTINATION lib COMPONENT NI)
