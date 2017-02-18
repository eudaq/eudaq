FIND_PACKAGE( ROOT REQUIRED)

SET(OfflineMon_name OfflineMonitor)

INCLUDE_DIRECTORIES( . inc ${ROOT_INCLUDE_DIR} ../../extern/rapidxml-1.13)

SET (OfflineMon_SOURCES
  src/OfflineMonitor.cxx
  src/CorrelationPlots.cpp
  src/CorrelationPlots_interface.cpp
  src/CorrelationVSTimePlots.cpp
  src/makeCorrelations.cpp
  src/Planes.cpp
  )

add_executable(${OfflineMon_name} ${OfflineMon_SOURCES} )

target_link_libraries(${OfflineMon_name} EUDAQ ${ROOT_LIBRARIES} ${ADDITIONAL_LIBRARIES})

INSTALL(TARGETS ${OfflineMon_name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
