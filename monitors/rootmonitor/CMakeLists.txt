cmake_dependent_option(EUDAQ_BUILD_ONLINE_ROOT_MONITOR "monitor/ROOTMonitor executable (requires ROOT)" OFF
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_BUILD_ONLINE_ROOT_MONITOR)
  message(STATUS "monitor/ROOTMonitor is NOT to be built (EUDAQ_BUILD_ONLINE_ROOT_MONITOR=OFF)")
  unset(EUDAQ_ROOT_MONITOR_LIBRARY CACHE)
  return()
endif()
message(STATUS "monitor/ROOTMonitor is to be built (EUDAQ_BUILD_ONLINE_ROOT_MONITOR=ON)")

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
if(WIN32)
  find_package(ROOT REQUIRED COMPONENTS libGui libCore libHist libRIO)
else()
  find_package(ROOT REQUIRED COMPONENTS Gui)
endif()

get_filename_component(EUDAQ_COMPONENT ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_ROOT_MONITOR_LIBRARY ${EUDAQ_COMPONENT} CACHE INTERNAL "eudaq root monitoring library")

set(EUDAQ_INCLUDE_DIRS
    ${EUDAQ_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${ROOT_INCLUDE_DIRS}
    CACHE INTERNAL "directory of eudaq include")

include_directories(${EUDAQ_INCLUDE_DIRS})

if(ROOT_USE_FILE)
  include(${ROOT_USE_FILE})
endif()

include_directories(. include)
include_directories(${CMAKE_BINARY_DIR})

aux_source_directory(src EUDAQ_COMPONENT_SRC)

root_generate_dictionary(R_ROOTMonitor eudaq/ROOTMonitor.hh eudaq/ROOTMonitorWindow.hh LINKDEF include/LinkDef.h)
list(APPEND EUDAQ_COMPONENT_SRC ${CMAKE_CURRENT_BINARY_DIR}/R_ROOTMonitor.cxx)
add_library(${EUDAQ_COMPONENT} SHARED ${EUDAQ_COMPONENT_SRC})

if(CMAKE_COMPILER_IS_GNUCC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0))
  set_property(TARGET ${EUDAQ_COMPONENT} PROPERTY CXX_STANDARD 14)
endif()

target_link_libraries(${EUDAQ_COMPONENT} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES})

install(TARGETS ${EUDAQ_COMPONENT}
  EXPORT eudaqTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

if(ROOT_VERSION_MAJOR GREATER 5)
  set(ROOTProducer_PCM ${CMAKE_CURRENT_BINARY_DIR}/libeudaq_R_ROOTMonitor_rdict.pcm)
  set(ROOTProducer_MAP ${CMAKE_CURRENT_BINARY_DIR}/libeudaq_R_ROOTMonitor.rootmap)
  install(FILES
    ${ROOTProducer_PCM}
    ${ROOTProducer_MAP}
    DESTINATION lib)
endif()

file(GLOB INC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/eudaq/*.hh")
install(FILES ${INC_FILES} DESTINATION include/eudaq)
