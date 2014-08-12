set(name "CMSPixelProducer.exe")
set(sourcefiles src/CMSPixelProducer.cxx src/CMSPixelController.cc)

SET(PXAR_PATH /home/cmspix/arthur/pxar)

INCLUDE_DIRECTORIES(include ${PXAR_INCLUDE_DIR}/api ${PXAR_INCLUDE_DIR}/utils ${PXAR_PATH}/util)
add_executable(${name} ${sourcefiles})

target_link_libraries(${name} EUDAQ ${EUDAQ_THREADS_LIB} ${PXAR_LIBRARY} ${PXARUTIL_LIBRARY})
MESSAGE(STATUS "Pixar library: ${PXAR_LIBRARY}")

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
