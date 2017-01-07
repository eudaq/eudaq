option(EUDAQ_MODULE_BUILD_TEST "build module_test" ON)
if(NOT EUDAQ_MODULE_BUILD_TEST)
  return()
endif()

get_filename_component(MODULE_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${MODULE_NAME})
# aux_source_directory(src MODULE_SRC)
set(MODULE_SRC
  src/TestProducer.cc
  )

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
