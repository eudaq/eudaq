OPTION(BUILD_onlinemon "Compile Online Monitor executable (requires ROOT)?" ON)
IF(BUILD_onlinemon)  
  LIST(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
  FIND_PACKAGE( ROOT REQUIRED COMPONENTS Gui)
  INCLUDE(${ROOT_USE_FILE})
  INCLUDE_DIRECTORIES( . include)

  IF(${ROOT_VERSION_MAJOR} GREATER 5)
    ADD_DEFINITIONS(-DEUDAQ_LIB_ROOT6)
  ENDIF()
  
  SET(OnlineMon_SOURCES
    src/OnlineMon.cxx
    src/OnlineMonWindow.cc
    src/BaseCollection.cc
    src/CheckEOF.cc
    src/CorrelationCollection.cc
    src/CorrelationHistos.cc
    src/EUDAQMonitorCollection.cc
    src/EUDAQMonitorHistos.cc
    src/EventSanityChecker.cc
    src/HitmapCollection.cc
    src/HitmapHistos.cc
    src/MonitorPerformanceCollection.cc
    src/MonitorPerformanceHistos.cc
    src/ParaMonitorCollection.cc
    src/OnlineMonConfiguration.cc
    src/SimpleStandardEvent.cc
    src/SimpleStandardPlane.cc
    )

  SET(OnlineMon_HEADERS 
    include/OnlineMonWindow.hh
    include/SimpleStandardHit.hh
    include/SimpleStandardCluster.hh
    include/SimpleStandardPlane.hh
    include/OnlineMonConfiguration.hh
    include/CheckEOF.hh
    )

  SET(OnlineMon_DICT ${CMAKE_CURRENT_BINARY_DIR}/OnlineMon_ROOT.cxx)
  SET(OnlineMon_PCM ${CMAKE_CURRENT_BINARY_DIR}/libOnlineMon_ROOT_rdict.pcm)
  SET(OnlineMon_MAP ${CMAKE_CURRENT_BINARY_DIR}/libOnlineMon_ROOT.rootmap)
  ROOT_GENERATE_DICTIONARY(OnlineMon_ROOT ${OnlineMon_HEADERS})

  ADD_EXECUTABLE(OnlineMon.exe ${OnlineMon_SOURCES} ${OnlineMon_DICT})
  TARGET_LINK_LIBRARIES(OnlineMon.exe EUDAQ ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} ${ADDITIONAL_LIBRARIES})

  INSTALL(TARGETS OnlineMon.exe
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

  IF(${ROOT_VERSION_MAJOR} GREATER 5)
    INSTALL(FILES
      ${OnlineMon_PCM}
      ${OnlineMon_MAP}
      DESTINATION bin
      )
  ENDIF()
  
ENDIF()
