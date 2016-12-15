option(EUDAQ_MODULE_STD "build module_std" ON)
if(NOT EUDAQ_MODULE_STD)
  return()
endif()

get_filename_component(MODULE_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${MODULE_NAME})
aux_source_directory(src MODULE_SRC)
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${PROJECT_NAME})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
