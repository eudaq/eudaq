include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_LCIO_LIBRARY)
# TODO:  list(REMOVE_ITEM MODULE_SRC src/NiRawEvent2LCEventConverter.cc)
endif()

if(NOT USER_ITS3_BUILD)
  list(REMOVE_ITEM MODULE_SRC src/*.cc)
endif()

find_package(ROOT QUIET)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY}
  ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES}
  ${ROOT_LIBRARIES} )

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
