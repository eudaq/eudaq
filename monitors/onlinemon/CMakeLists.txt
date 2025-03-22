cmake_dependent_option(EUDAQ_BUILD_STDEVENT_MONITOR "monitor/StdEventMonitor executable (requires ROOT)" OFF
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_BUILD_STDEVENT_MONITOR)
    message(STATUS "monitor/StdEventMonitor is NOT to be built (EUDAQ_BUILD_STDEVENT_MONITOR=OFF)")
  return()
endif()
message(STATUS "monitor/StdEventMonitor is to be built (EUDAQ_BUILD_STDEVENT_MONITOR=ON)")

find_package(ROOT REQUIRED COMPONENTS Core Gui Hist)

set(THISMON StdEventMonitor)
set(THISMON_DICT_CXX ${CMAKE_CURRENT_BINARY_DIR}/${THISMON}_ROOT.cxx)
root_generate_dictionary(${THISMON}_ROOT include/OnlineMonWindow.hh)

aux_source_directory(src THISMON_SRC)
add_executable(${THISMON} ${THISMON_SRC} ${THISMON_DICT_CXX})

target_link_libraries(${THISMON} ${EUDAQ_CORE_LIBRARY} ROOT::Core ROOT::Gui ROOT::Hist)
target_include_directories(${THISMON} PRIVATE . include)

install(TARGETS ${THISMON}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
  
set(StdEvtMonitor_PCM ${CMAKE_CURRENT_BINARY_DIR}/libStdEventMonitor_ROOT_rdict.pcm)
set(StdEvtMonitor_MAP ${CMAKE_CURRENT_BINARY_DIR}/libStdEventMonitor_ROOT.rootmap)
install(FILES
 ${StdEvtMonitor_PCM}
 ${StdEvtMonitor_MAP}
DESTINATION lib)

