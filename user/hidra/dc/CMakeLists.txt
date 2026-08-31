option(USER_HIDRA_DC_ROOT_OUTPUT "Build HiDRa DataCollector ROOT ntuple writer" ON)

include_directories(include /usr/include)
link_directories(/usr/lib)
aux_source_directory(src DC_SRC)
list(APPEND DC_SRC src/HidraNullFileWriter.cc)

if(USER_HIDRA_DC_ROOT_OUTPUT)
  find_package(ROOT QUIET COMPONENTS Core Tree RIO)
  if(ROOT_FOUND)
    include_directories(${ROOT_INCLUDE_DIRS})
    if(ROOT_USE_FILE)
      include(${ROOT_USE_FILE})
    endif()
    set(DC_ROOT_LIBS ${ROOT_LIBRARIES})
  else()
    message(WARNING "ROOT not found; disabling USER_HIDRA_DC_ROOT_OUTPUT")
    set(USER_HIDRA_DC_ROOT_OUTPUT OFF)
  endif()
endif()

# if(NOT EUDAQ_TTREE_LIBRARY)
#   list(REMOVE_ITEM MODULE_SRC src/QTPProducerEvent2TTreeEventConverter.cc)
#   list(REMOVE_ITEM MODULE_SRC src/DRSProducerEvent2TTreeEventConverter.cc)
#   list(REMOVE_ITEM MODULE_SRC src/FERSProducerEvent2TTreeEventConverter.cc)
# endif()

add_library(${EUDAQ_MODULE}_dc SHARED ${DC_SRC})
target_link_libraries(${EUDAQ_MODULE}_dc ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY} ${EUDAQ_ROOT_MONITOR_LIBRARY} ${EUDAQ_MODULE}_common)

if(USER_HIDRA_DC_ROOT_OUTPUT)
  target_link_libraries(${EUDAQ_MODULE}_dc ${DC_ROOT_LIBS})
endif()

install(TARGETS
  ${EUDAQ_MODULE}_dc
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

