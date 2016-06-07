OPTION(BUILD_offlinemon "Compile Offline Monitor executable (requires ROOT)?" OFF)
IF(BUILD_offlinemon)
  FIND_PACKAGE(ROOT REQUIRED)
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

  ADD_EXECUTABLE(${OfflineMon_name} ${OfflineMon_SOURCES} )

  TARGET_LINK_LIBRARIES(${OfflineMon_name} EUDAQ ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} ${ADDITIONAL_LIBRARIES})

  INSTALL(TARGETS ${OfflineMon_name}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF(BUILD_offlinemon)