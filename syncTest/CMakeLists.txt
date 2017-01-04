# build Sync Test
option(BUILD_SyncTest "Compile Sync test" OFF)
IF(BUILD_SyncTest)
  SET(SyncTest_name SyncTest)

  INCLUDE_DIRECTORIES( . inc )

  SET (SyncTest_SOURCES
    src/paketProducer.cc
    src/dataQueue.cc
    src/syncTest.cxx
    )

  add_executable(${SyncTest_name} ${SyncTest_SOURCES} )

  target_link_libraries(${SyncTest_name} ${EUDAQ_CORE_LIBRARY} ${ADDITIONAL_LIBRARIES})

  INSTALL(TARGETS ${SyncTest_name}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF(BUILD_SyncTest)
