option(USE_ROOT "Compiling main library using ROOT" OFF)
if (USE_ROOT)
   FIND_PACKAGE( ROOT REQUIRED )
   if (ROOT_FOUND)
     INCLUDE_DIRECTORIES( ${ROOT_INCLUDE_DIR} )
     SET(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}  ${ROOT_LIBRARIES})
     ADD_DEFINITIONS(-DROOT_FOUND)
   endif (ROOT_FOUND)
endif (USE_ROOT)

AUX_SOURCE_DIRECTORY( src library_sources )
AUX_SOURCE_DIRECTORY( plugins plugins_sources )

ADD_LIBRARY( ${PROJECT_NAME} SHARED ${library_sources}  ${plugins_sources})
SOURCE_GROUP(plugins .*Plugin.*cc)
SOURCE_GROUP(FileIO .*File.*cc)
SOURCE_GROUP(TCPIP .*Transport.*cc)
SOURCE_GROUP(TCPIP FILES src\\Serializable.cc src\\Producer.cc src\\CommandReceiver.cc src\\DataCollector.cc src\\DataSender.cc  src\\LogCollector.cc)
SOURCE_GROUP(Event .*Event.*cc)
SOURCE_GROUP(Utilities FILES src\\Utils.cc src\\Time.cc src\\Mutex.cc src\\EudaqThread.cc)

target_link_libraries( ${PROJECT_NAME} ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})


INSTALL(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
