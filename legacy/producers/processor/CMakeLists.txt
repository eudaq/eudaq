GET_FILENAME_COMPONENT(DIRNAME ${CMAKE_CURRENT_LIST_DIR} NAME)


# AUX_SOURCE_DIRECTORY(src PS_PLUGIN_SRC)
SET(PS_PLUGIN_SRC
  src/ExamplePS.cc
  src/EventFileReaderPS.cc
  src/DetEventUnpackInsertTimestampPS.cc
  # src/SyncByTimestampPS.cc
  src/EventReceiverPS.cc
  src/EventSenderPS.cc
  src/TimeTriggerPS.cc
  src/DummyDevicePS.cc
  src/RandomDelayPS.cc
  src/DummySystemPS.cc
  src/OldEventFilterPS.cc
  src/UnpackSyncEventPS.cc
  # src/FileWriterProcessor.cc
  # src/SyncByEventnumberPS.cc
  )


INCLUDE_DIRECTORIES(include)

SET(PS_PLUGIN_NAME module_ps_example)


ADD_LIBRARY(${PS_PLUGIN_NAME} SHARED ${PS_PLUGIN_SRC})
TARGET_LINK_LIBRARIES(${PS_PLUGIN_NAME} EUDAQ)

SET(PRODUCER_NAME "processortest")
SET(PRODUCER_SRC
  src/testmain.cxx
  )

ADD_EXECUTABLE(${PRODUCER_NAME} ${PRODUCER_SRC})

TARGET_LINK_LIBRARIES(${PRODUCER_NAME} EUDAQ)


INSTALL(TARGETS
  ${PS_PLUGIN_NAME}
  ${PRODUCER_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  
  )


IF(UNIX)
LIST(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
FIND_PACKAGE(ROOT REQUIRED)
INCLUDE(${ROOT_USE_FILE})

SET(PLUGIN_ROOT psmanroot)
SET(PLUGIN_DICT ${PLUGIN_ROOT}_ROOT)
SET(PLUGIN_DICT_CXX ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_DICT}.cxx)
SET(PLUGIN_DICT_PCM ${CMAKE_CURRENT_BINARY_DIR}/lib${PLUGIN_DICT}_rdict.pcm)
SET(PLUGIN_DICT_MAP ${CMAKE_CURRENT_BINARY_DIR}/lib${PLUGIN_DICT}.rootmap)

ROOT_GENERATE_DICTIONARY(${PLUGIN_DICT} ${PLUGIN_INC} LINKDEF include/LinkDef.hh)
ADD_LIBRARY(${PLUGIN_ROOT} SHARED ${PLUGIN_DICT_CXX})
TARGET_LINK_LIBRARIES(${PLUGIN_ROOT} EUDAQ ${PLUGIN_NAME})

INSTALL(TARGETS
  ${PLUGIN_ROOT}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

INSTALL(FILES
  ${PLUGIN_DICT_PCM}
  ${PLUGIN_DICT_MAP}
  DESTINATION lib
  )

ENDIF()
