set(name "CMSPixelProducer.exe")
set(sourcefiles src/CMSPixelProducer.cxx src/CMSPixelController.cc)

INCLUDE_DIRECTORIES(include ${PXAR_INCLUDE_DIR})
add_executable(${name} ${sourcefiles})

target_link_libraries(${name} EUDAQ ${EUDAQ_THREADS_LIB} ${PXAR_LIB})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
