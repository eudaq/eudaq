GET_FILENAME_COMPONENT(DIRNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
SET(PLUGIN_NAME plugin.${DIRNAME})
SET_PROPERTY(GLOBAL APPEND PROPERTY CONVERTER_PLUGIN ${PLUGIN_NAME})

INCLUDE_DIRECTORIES(include)

SET(PLUGIN_SRC
  src/MVDConverterPlugin.cc
  )
ADD_LIBRARY(${PLUGIN_NAME} SHARED ${PLUGIN_SRC})
INSTALL(TARGETS ${PLUGIN_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


set(name "MVDProducer.exe")
IF(NOT IGNORE_KNOWN_ISSUES)
  IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
    MESSAGE(WARNING "There are known issues compiling ${name} (see https://github.com/eudaq/eudaq/issues/74). Set the IGNORE_KNOWN_ISSUES option to configure this producer to be build anyway.")
  ENDIF(NOT BUILD_allproducer)
ELSE(NOT IGNORE_KNOWN_ISSUES)
  set(sourcefiles src/MVDProducer.cxx src/MVDController.cc src/ModV551.cc src/ModV550.cc)
  set(ext_libraries "vme" )
  set(ext_lib_paths "" )
  set(ext_inc_paths "" )

  ADD_DEFINITIONS(-DVME_TRACE=0)
  LINK_DIRECTORIES( ${ext_lib_paths} )
  INCLUDE_DIRECTORIES( include ${ext_inc_paths})
  ADD_EXECUTABLE(${name} ${sourcefiles})

  TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})

  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF(NOT IGNORE_KNOWN_ISSUES)