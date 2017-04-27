GET_FILENAME_COMPONENT(DIRNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
SET(PLUGIN_NAME plugin.${DIRNAME})
SET_PROPERTY(GLOBAL APPEND PROPERTY CONVERTER_PLUGIN ${PLUGIN_NAME})

INCLUDE_DIRECTORIES(include)

SET(PLUGIN_SRC
  src/TAKIConverterPlugin.cc
  )
ADD_LIBRARY(${PLUGIN_NAME} SHARED ${PLUGIN_SRC})
INSTALL(TARGETS ${PLUGIN_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  )



set(name "TakiProducer.exe")
IF(NOT IGNORE_KNOWN_ISSUES AND WIN32) # issues compiling under WINDOWS only
  IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
    MESSAGE(WARNING "There are known issues compiling ${name} on your platform. Set the IGNORE_KNOWN_ISSUES option to configure this producer to be build anyway.")
  ENDIF(NOT BUILD_allproducer)
ELSE(NOT IGNORE_KNOWN_ISSUES AND WIN32)
  set(sourcefiles src/TakiProducer.cxx)
  set(ext_libraries "" )
  set(ext_lib_paths "" )
  set(ext_inc_paths "" )

  LINK_DIRECTORIES( ${ext_lib_paths} )
  INCLUDE_DIRECTORIES( include ${ext_inc_paths})
  ADD_EXECUTABLE(${name} ${sourcefiles})
  ADD_EXECUTABLE(TakiProducerFAKEDATA.exe src/TakiProducerFAKEDATA.cxx)

  TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})
  TARGET_LINK_LIBRARIES(TakiProducerFAKEDATA.exe   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})

  INSTALL(TARGETS ${name} TakiProducerFAKEDATA.exe
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF(NOT IGNORE_KNOWN_ISSUES AND WIN32)