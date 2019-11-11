#option(EUDAQ_MODULE_BUILD_STD "build module_std" ON)
#if(NOT EUDAQ_MODULE_BUILD_STD)
#  return()
#endif()

#get_filename_component(MODULE_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
#set(EUDAQ_MODULE module_${MODULE_NAME})

include_directories(include)
aux_source_directory(src MODULE_SRC)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY})


install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
