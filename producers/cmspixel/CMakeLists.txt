set(name "CMSPixelProducer.exe")

IF("$ENV{PXAR_PATH}" STREQUAL "")
  MESSAGE(FATAL_ERROR "Could not find pxar. Please setup the environment variable PXAR_PATH to point to your pxar installation.")
ELSE("$ENV{PXAR_PATH}" STREQUAL "")
  MESSAGE(STATUS "pxarCore path: $ENV{PXAR_PATH}")
  FIND_LIBRARY(PXAR_LIBRARY
    NAMES libpxar.so
    PATHS "$ENV{PXAR_PATH}/lib")
  IF(PXAR_LIBRARY)
    MESSAGE(STATUS "pxarCore library: ${PXAR_LIBRARY}")
  ELSE(PXAR_LIBRARY)
    MESSAGE(FATAL_ERROR "Could not find pxarCore library. Please compile pxar.")
  ENDIF(PXAR_LIBRARY)
ENDIF("$ENV{PXAR_PATH}" STREQUAL "")

set(sourcefiles src/CMSPixelProducer.cxx src/CMSPixelEvtMonitor.cxx src/CMSPixelConfigParser.cxx src/CMSPixelEvtWriter.cxx)

include(FindROOT.cmake)


INCLUDE_DIRECTORIES(include $ENV{PXAR_PATH}/core/api $ENV{PXAR_PATH}/core/utils $ENV{PXAR_PATH}/util ${ROOT_INCLUDE_DIR})
add_executable(${name} ${sourcefiles})

target_link_libraries(${name} EUDAQ ${EUDAQ_THREADS_LIB} $ENV{PXAR_PATH}/lib/libpxar.so ${ROOT_LIBRARIES})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
