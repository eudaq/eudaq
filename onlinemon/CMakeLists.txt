FIND_PACKAGE( ROOT REQUIRED)

INCLUDE_DIRECTORIES( . include ${ROOT_INCLUDE_DIR} )

SET (OnlineMon_SOURCES
  src/OnlineMon.cxx
  src/OnlineMonWindow.cc
  src/BaseCollection.cc
  src/CheckEOF.cc
  src/CorrelationCollection.cc
  src/CorrelationHistos.cc
  src/EUDAQMonitorCollection.cc
  src/EUDAQMonitorHistos.cc
  src/EventSanityChecker.cc
  src/GraphWindow.cc
  src/HitmapCollection.cc
  src/HitmapHistos.cc
  src/MonitorPerformanceCollection.cc
  src/MonitorPerformanceHistos.cc
  src/OnlineHistograms.cc
  src/OnlineMonConfiguration.cc
  src/SimpleStandardEvent.cc
  src/SimpleStandardPlane.cc
  src/TGraphSet.cc
#  src/TreeHistoRelation.cc
  )

# fill list of header files 
set(OnlineMon_HEADERS 
  OnlineMonWindow.hh 
  CheckEOF.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/onlinemon/include/ ${PROJECT_SOURCE_DIR}/onlinemon ${PROJECT_SOURCE_DIR}/main/include ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
set(OnlineMon_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/OnlineMonDict.cxx) 
ROOT_GENERATE_DICTIONARY_OLD("${OnlineMon_HEADERS}" "${OnlineMon_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")

add_executable(OnlineMon.exe ${OnlineMon_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/OnlineMonDict.cxx)

target_link_libraries(OnlineMon.exe EUDAQ ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} )

INSTALL(TARGETS OnlineMon.exe
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
