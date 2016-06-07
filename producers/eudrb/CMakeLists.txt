GET_FILENAME_COMPONENT(DIRNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
SET(PLUGIN_NAME plugin.${DIRNAME})
SET_PROPERTY(GLOBAL APPEND PROPERTY CONVERTER_PLUGIN ${PLUGIN_NAME})

INCLUDE_DIRECTORIES(include)

SET(PLUGIN_SRC
  src/EUDRBConverterPlugin.cc
  src/EUDRBEvent.cc
  )
ADD_LIBRARY(${PLUGIN_NAME} SHARED ${PLUGIN_SRC})


ADD_EXECUTABLE(MagicLogBook src/MagicLogBook.cxx)
TARGET_LINK_LIBRARIES(MagicLogBook EUDAQ ${PLUGIN_NAME})

ADD_EXECUTABLE(TestMonitor src/TestMonitor.cxx)
TARGET_LINK_LIBRARIES(TestMonitor EUDAQ ${PLUGIN_NAME})


INSTALL(TARGETS ${PLUGIN_NAME} MagicLogBook TestMonitor
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  )



SET(name "EUDRBProducer")
IF(NOT IGNORE_KNOWN_ISSUES AND WIN32) # issues compiling under WINDOWS only
  IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
    MESSAGE(WARNING "There are known issues compiling ${name} on your platform. Set the IGNORE_KNOWN_ISSUES option to configure this producer to be build anyway.")
  ENDIF(NOT BUILD_allproducer)
ELSE(NOT IGNORE_KNOWN_ISSUES AND WIN32)
  ADD_SUBDIRECTORY(vmelib)

  INCLUDE_DIRECTORIES(vmelib/include)
  SET(sourcefiles
    src/EUDRBProducer.cxx
    src/EUDRBController.cc
    )

  ADD_DEFINITIONS("-DVME_TRACE=0")
  ADD_EXECUTABLE(${name} ${sourcefiles})

  TARGET_LINK_LIBRARIES(${name}  ${PLUGIN_NAME} EUDAQ ${EUDAQ_THREADS_LIB} vmelib)

  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF(NOT IGNORE_KNOWN_ISSUES AND WIN32)
