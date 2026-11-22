include_directories(
  include
  ../common/include
  /usr/local/include
  /usr/include)
link_directories(/usr/local/lib)
link_directories(/usr/lib)

aux_source_directory(src FERS2_SRC)
list(REMOVE_ITEM FERS2_SRC src/Example_usage.cc)

find_library(CAEN_FERS_LIBRARY NAMES caenferslib PATHS /usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu)
if(NOT CAEN_FERS_LIBRARY)
  message(FATAL_ERROR "Could not find caenferslib. Install CAEN FERSlib or update the library search path.")
endif()

add_library(${EUDAQ_MODULE}_fers2 SHARED ${FERS2_SRC})
target_link_libraries(${EUDAQ_MODULE}_fers2 ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY} ${EUDAQ_ROOT_MONITOR_LIBRARY} ${EUDAQ_MODULE}_common ${CAEN_FERS_LIBRARY})

install(TARGETS
  ${EUDAQ_MODULE}_fers2
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
