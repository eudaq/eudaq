if(WIN32)
  message("CMS-HGCal monitor is not to be built on windows")
  return()
endif()

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT)

cmake_dependent_option(EUDAQ_BUILD_CMSHGCAL_MONITOR "build CMS-HGCal monitor executable" ON
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_BUILD_CMSHGCAL_MONITOR)
  message("CMS-HGCal monitor is not to be built")
  return()
endif()

if(WIN32)
  find_package(ROOT REQUIRED COMPONENTS libGui libCore libHist libRIO)
else()
  find_package(ROOT REQUIRED COMPONENTS Gui)
endif()

include_directories(${ROOT_INCLUDE_DIR})
if(ROOT_USE_FILE)
  include(${ROOT_USE_FILE})
endif()

include_directories(. include)
include_directories(${CMAKE_BINARY_DIR})

if(ROOT_VERSION_MAJOR GREATER 5)
  add_definitions(-DEUDAQ_LIB_ROOT6)
endif()

set(THISMON CMSHGCalMonitor)

set(THISMON_DICT_CXX ${CMAKE_CURRENT_BINARY_DIR}/${THISMON}_ROOT.cxx)
root_generate_dictionary(${THISMON}_ROOT include/OnlineMonWindow.hh)

aux_source_directory(src THISMON_SRC)
add_executable(${THISMON} ${THISMON_SRC} ${THISMON_DICT_CXX})

if(CMAKE_COMPILER_IS_GNUCC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0))
  set_property(TARGET ${THISMON} PROPERTY CXX_STANDARD 14)
endif()
target_link_libraries(${THISMON} ${EUDAQ_CORE_LIBRARY} ${ROOT_LIBRARIES})

install(TARGETS ${THISMON}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

# FIND_PACKAGE(ROOT REQUIRED)

# INCLUDE_DIRECTORIES( . include ${ROOT_INCLUDE_DIR} )

# SET (HgcOnlineMon_SOURCES
#   src/OnlineMon.cxx
#   src/OnlineMonWindow.cc
#   src/BaseCollection.cc
#   src/HexagonCollection.cc
#   src/HexagonHistos.cc
#   src/HexagonCorrelationCollection.cc
#   src/HexagonCorrelationHistos.cc  
#   src/AhcalCollection.cc
#   src/AhcalHistos.cc
#   src/HitmapCollection.cc
#   src/HitmapHistos.cc
#   src/CorrelationCollection.cc
#   src/CorrelationHistos.cc
#   src/DATURAToHGCALCorrelationCollection.cc
#   src/DATURAToHGCALCorrelationHistos.cc
#   #src/DWCToHGCALCorrelationCollection.cc
#   #src/DWCToHGCALCorrelationHistos.cc
#   src/WireChamberCollection.cc
#   src/WireChamberHistos.cc
#   src/TDCHitsCollection.cc
#   src/TDCHitsHistos.cc
#   src/DigitizerCollection.cc
#   src/DigitizerHistos.cc  
#   src/WireChamberCorrelationCollection.cc
#   src/WireChamberCorrelationHistos.cc  
#   src/CheckEOF.cc
#   src/EUDAQMonitorCollection.cc
#   src/EUDAQMonitorHistos.cc
#   src/EventSanityChecker.cc
#   src/GraphWindow.cc
#   src/OnlineHistograms.cc
#   src/OnlineMonConfiguration.cc
#   src/SimpleStandardEvent.cc
#   src/SimpleStandardPlane.cc
#   src/TGraphSet.cc
#   src/overFlowBins.cc
# #  src/TreeHistoRelation.cc
#   )

# # fill list of header files 
# set(HgcOnlineMon_HEADERS 
#   OnlineMonWindow.hh
#   SimpleStandardHit.hh
#   SimpleStandardCluster.hh
#   SimpleStandardPlane.hh
#   OnlineMonConfiguration.hh
#   CheckEOF.hh
# )

# SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/monitors/cmshgcal_onlinemon/include/ ${PROJECT_SOURCE_DIR}/monitors/cmshgcal_onlinemon ${PROJECT_SOURCE_DIR}/main/include ${ROOT_INCLUDE_DIR} )

# # set everything needed for the root dictonary and create the
# # dictionary
# set(HgcOnlineMon_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/HgcOnlineMonDict.cxx) 
# ROOT_GENERATE_DICTIONARY_OLD("${HgcOnlineMon_HEADERS}" "${HgcOnlineMon_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")

# add_executable(HgcOnlineMon.exe ${HgcOnlineMon_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/HgcOnlineMonDict.cxx)

# target_link_libraries(HgcOnlineMon.exe EUDAQ ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} ${ADDITIONAL_LIBRARIES})

# INSTALL(TARGETS HgcOnlineMon.exe
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib)
