SET(SyncTest_name SyncTest)

INCLUDE_DIRECTORIES( . inc )

SET (SyncTest_SOURCES
  src/*.cc
  src/*.cxx
  )

add_executable(${SyncTest_name} ${SyncTest_SOURCES} )

target_link_libraries(${SyncTest_name} EUDAQ ${ADDITIONAL_LIBRARIES})

INSTALL(TARGETS ${SyncTest_name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
