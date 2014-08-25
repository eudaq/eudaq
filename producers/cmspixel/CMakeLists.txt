set(name "CMSPixelProducer.exe")
set(sourcefiles src/CMSPixelProducer.cxx src/CMSPixelController.cc src/CMSPixelEvtMonitor.cxx)

include(FindROOT.cmake)
MESSAGE("-- PXAR_PATH ${PXAR_PATH}")

INCLUDE_DIRECTORIES(include ${PXAR_PATH}/core/api ${PXAR_PATH}/core/utils ${PXAR_PATH}/util ${ROOT_INCLUDE_DIR})
add_executable(${name} ${sourcefiles})


target_link_libraries(${name} EUDAQ ${EUDAQ_THREADS_LIB} ${PXAR_LIBRARY} ${ROOT_LIBRARIES})
MESSAGE(STATUS "Pixar library: ${PXAR_LIBRARY}")

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
