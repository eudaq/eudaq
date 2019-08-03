find_package(ROOT REQUIRED COMPONENTS Core Gui)


INCLUDE(${ROOT_USE_FILE})

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
  ${CMAKE_CURRENT_SOURCE_DIR}/include/OnlineMonWindow.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/include/SimpleStandardHit.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/include/SimpleStandardCluster.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/include/SimpleStandardPlane.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/include/OnlineMonConfiguration.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/include/CheckEOF.hh
  )


#This is an ugly workaround...should be all automatically done by the dictionary macro from root
get_target_property(OnlineMon_EUDAQ_HEADERS EUDAQ INCLUDE_DIRECTORIES)
#message(${OnlineMon_EUDAQ_HEADERS})
#include_directories(${OnlineMon_EUDAQ_HEADERS})
include_directories("${CMAKE_SOURCE_DIR}/main/include")

SET(OnlineMon_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/OnlineMonDict.cxx) 
# FIXME Workaround to be replaced as soon as there is some solution within ROOT
# https://sft.its.cern.ch/jira/browse/ROOT-8575
# https://sft.its.cern.ch/jira/browse/ROOT-7549
ROOT_GENERATE_DICTIONARY(OnlineMonDict ${OnlineMon_HEADERS} )

ADD_EXECUTABLE(OnlineMon.exe ${OnlineMon_SOURCES} ${OnlineMon_DICTIONARY})
TARGET_INCLUDE_DIRECTORIES(OnlineMon.exe PRIVATE 
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/monitors/onlinemon/include>
)

message(${ROOT_LIBRARIES})

TARGET_LINK_LIBRARIES(OnlineMon.exe EUDAQ ${ROOT_LIBRARIES} ${ADDITIONAL_LIBRARIES})

#get_target_property(OnlineMon_EUDAQ_HEADERS EUDAQ INCLUDE_DIRECTORIES)
#message(${OnlineMon_EUDAQ_HEADERS})
#get_target_property(OnlineMon_EUDAQ_HEADERS OnlineMon.exe INCLUDE_DIRECTORIES)
#message(${OnlineMon_EUDAQ_HEADERS})

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

IF(${ROOT_VERSION} GREATER 5)
  SET(OnlineMon_PCM ${CMAKE_CURRENT_BINARY_DIR}/OnlineMonDict_rdict.pcm) 
  INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libOnlineMonDict_rdict.pcm
    DESTINATION bin COMPONENT ONLINEMON
    )
    ADD_DEFINITIONS( -DEUDAQ_LIB_ROOT6 )
ENDIF(${ROOT_VERSION} GREATER 5)
