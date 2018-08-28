if(WIN32)
  message("StdEvent monitor is not to be built on windows")
  return()
endif()

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT)

cmake_dependent_option(EUDAQ_BUILD_STDEVENT_MONITOR "build StdEvent monitor executable" ON
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_BUILD_STDEVENT_MONITOR)
  message("StdEvent monitor is not to be built")
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

set(THISMON StdEventMonitor)

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
