FIND_PACKAGE(ROOT REQUIRED)

INCLUDE_DIRECTORIES( . include ${ROOT_INCLUDE_DIR} )

SET (HgcOnlineMon_SOURCES
  src/OnlineMon.cxx
  src/OnlineMonWindow.cc
  src/BaseCollection.cc
  src/HexagonCollection.cc
  src/HexagonHistos.cc
  src/HexagonCorrelationCollection.cc
  src/HexagonCorrelationHistos.cc  
  src/AhcalCollection.cc
  src/AhcalHistos.cc
  src/HitmapCollection.cc
  src/HitmapHistos.cc
  src/CorrelationCollection.cc
  src/CorrelationHistos.cc
  src/DATURAToHGCALCorrelationCollection.cc
  src/DATURAToHGCALCorrelationHistos.cc
  src/DWCToHGCALCorrelationCollection.cc
  src/DWCToHGCALCorrelationHistos.cc
  src/WireChamberCollection.cc
  src/WireChamberHistos.cc
  src/WireChamberCorrelationCollection.cc
  src/WireChamberCorrelationHistos.cc  
  src/CheckEOF.cc
  src/EUDAQMonitorCollection.cc
  src/EUDAQMonitorHistos.cc
  src/EventSanityChecker.cc
  src/GraphWindow.cc
  src/OnlineHistograms.cc
  src/OnlineMonConfiguration.cc
  src/SimpleStandardEvent.cc
  src/SimpleStandardPlane.cc
  src/TGraphSet.cc
  src/overFlowBins.cc
#  src/TreeHistoRelation.cc
  )

# fill list of header files 
set(HgcOnlineMon_HEADERS 
  OnlineMonWindow.hh
  SimpleStandardHit.hh
  SimpleStandardCluster.hh
  SimpleStandardPlane.hh
  OnlineMonConfiguration.hh
  CheckEOF.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/monitors/cmshgcal_onlinemon/include/ ${PROJECT_SOURCE_DIR}/monitors/cmshgcal_onlinemon ${PROJECT_SOURCE_DIR}/main/include ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
set(HgcOnlineMon_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/HgcOnlineMonDict.cxx) 
ROOT_GENERATE_DICTIONARY_OLD("${HgcOnlineMon_HEADERS}" "${HgcOnlineMon_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")

add_executable(HgcOnlineMon.exe ${HgcOnlineMon_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/HgcOnlineMonDict.cxx)

target_link_libraries(HgcOnlineMon.exe EUDAQ ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} ${ADDITIONAL_LIBRARIES})

INSTALL(TARGETS HgcOnlineMon.exe
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
