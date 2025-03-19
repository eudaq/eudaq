cmake_dependent_option(EUDAQ_BUILD_STDEVENT_MONITOR "monitor/StdEventMonitor executable (requires ROOT)" OFF
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_BUILD_STDEVENT_MONITOR)
    message(STATUS "monitor/StdEventMonitor is NOT to be built (EUDAQ_BUILD_STDEVENT_MONITOR=OFF)")
  return()
endif()
message(STATUS "monitor/StdEventMonitor is to be built (EUDAQ_BUILD_STDEVENT_MONITOR=ON)")

if(WIN32)
  find_package(ROOT REQUIRED COMPONENTS libGui libCore libHist libRIO)
else()
  find_package(ROOT REQUIRED COMPONENTS Gui)
endif()

include_directories(${ROOT_INCLUDE_DIR})
include_directories(. include)
include_directories(${CMAKE_BINARY_DIR})

if(ROOT_VERSION_MAJOR GREATER 5)
  add_definitions(-DEUDAQ_LIB_ROOT6)
endif()

set(THISMON StdEventMonitor)
set(THISMON_DICT_CXX ${CMAKE_CURRENT_BINARY_DIR}/${THISMON}_ROOT.cxx)
root_generate_dictionary(${THISMON}_ROOT include/OnlineMonWindow.hh)

aux_source_directory(src THISMON_SRC)
add_executable(${THISMON} ${THISMON_SRC} ${THISMON_DICT_CXX})

target_link_libraries(${THISMON} ${EUDAQ_CORE_LIBRARY} ${ROOT_LIBRARIES})

install(TARGETS ${THISMON}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
