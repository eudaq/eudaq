find_package(ROOT QUIET)

cmake_dependent_option(EUDAQ_BUILD_ONLINE_ROOT_MONITOR "monitor/ROOTMonitor executable (requires ROOT)" OFF
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_BUILD_ONLINE_ROOT_MONITOR)
  message(STATUS "monitor/ROOTMonitor is NOT to be built (EUDAQ_BUILD_ONLINE_ROOT_MONITOR=OFF)")
  unset(EUDAQ_ROOT_MONITOR_LIBRARY CACHE)
  return()
endif()
message(STATUS "monitor/ROOTMonitor is to be built (EUDAQ_BUILD_ONLINE_ROOT_MONITOR=ON)")

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT REQUIRED COMPONENTS Core Gui Hist RIO)

get_filename_component(EUDAQ_COMPONENT ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_ROOT_MONITOR_LIBRARY ${EUDAQ_COMPONENT} CACHE INTERNAL "eudaq root monitoring library")


aux_source_directory(src EUDAQ_COMPONENT_SRC)

add_library(${EUDAQ_COMPONENT} SHARED ${EUDAQ_COMPONENT_SRC})

target_include_directories(${EUDAQ_COMPONENT} PUBLIC . include)
target_link_libraries(${EUDAQ_COMPONENT} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ROOT::Core ROOT::Gui ROOT::Hist ROOT::RIO)

root_generate_dictionary(rootmonitorDict eudaq/ROOTMonitor.hh eudaq/ROOTMonitorWindow.hh
  LINKDEF include/LinkDef.h
    MODULE
    rootmonitor        
  )

install(TARGETS ${EUDAQ_COMPONENT}
  EXPORT eudaqTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


set(ROOTProducer_PCM ${CMAKE_CURRENT_BINARY_DIR}/librootmonitor_rdict.pcm)
set(ROOTProducer_MAP ${CMAKE_CURRENT_BINARY_DIR}/librootmonitor.rootmap)
install(FILES
  ${ROOTProducer_PCM}
  ${ROOTProducer_MAP}
  DESTINATION lib)

file(GLOB INC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/eudaq/*.hh")
install(FILES ${INC_FILES} DESTINATION include/eudaq)
