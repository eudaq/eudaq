get_filename_component(EUDAQ_MONITORNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_MONITORNAME})

option(EUDAQ_BUILD_CORRYMONITOR "build monitors/corrymonitor" OFF)

if(NOT EUDAQ_BUILD_CORRYMONITOR)
  message(STATUS "monitors/corrymonitor is NOT to be built (EUDAQ_BUILD_CORRYMONITOR=OFF)")
  return()
endif()
message(STATUS "monitors/corrymonitor is to be built (EUDAQ_BUILD_CORRYMONITOR=ON)")


aux_source_directory(src MODULE_SRC)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)