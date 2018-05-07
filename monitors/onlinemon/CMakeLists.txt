FIND_PACKAGE( ROOT REQUIRED)

INCLUDE_DIRECTORIES( . include ${ROOT_INCLUDE_DIR} )

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
  OnlineMonWindow.hh
  SimpleStandardHit.hh
  SimpleStandardCluster.hh
  SimpleStandardPlane.hh
  OnlineMonConfiguration.hh
  CheckEOF.hh
  )

SET(OnlineMon_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/OnlineMonDict.cxx) 
ADD_EXECUTABLE(OnlineMon.exe ${OnlineMon_SOURCES} ${OnlineMon_DICTIONARY})
TARGET_INCLUDE_DIRECTORIES(OnlineMon.exe PRIVATE 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
)
TARGET_LINK_LIBRARIES(OnlineMon.exe EUDAQ ${ROOT_LIBRARIES} ${ADDITIONAL_LIBRARIES})

#get_target_property(OnlineMon_EUDAQ_HEADERS EUDAQ INCLUDE_DIRECTORIES)
#message(${OnlineMon_EUDAQ_HEADERS})

# FIXME Workaround to be replaced as soon as there is some solution within ROOT
# https://sft.its.cern.ch/jira/browse/ROOT-8575
# https://sft.its.cern.ch/jira/browse/ROOT-7549
ROOT_GENERATE_DICTIONARY(OnlineMonDict ${OnlineMon_HEADERS} TARGETS EUDAQ)

INSTALL(TARGETS OnlineMon.exe
  EXPORT ${PROJECT_NAME}
  RUNTIME DESTINATION bin COMPONENT ONLINEMON
  LIBRARY DESTINATION lib COMPONENT ONLINEMON
  ARCHIVE DESTINATION lib COMPONENT ONLINEMON
  )

#export(TARGETS OnlineMon.exe
#  NAMESPACE EUDAQ::
#  APPEND FILE "${CMAKE_CURRENT_BINARY_DIR}/eudaqTargets.cmake"
#)

IF(found_root_major_vers GREATER 5)
  SET(OnlineMon_PCM ${CMAKE_CURRENT_BINARY_DIR}/OnlineMonDict_rdict.pcm) 
  INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/OnlineMonDict_rdict.pcm
    DESTINATION bin COMPONENT ONLINEMON
    )
ENDIF(found_root_major_vers GREATER 5)
