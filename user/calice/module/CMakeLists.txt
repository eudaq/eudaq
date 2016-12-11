cmake_dependent_option(USER_CALICE_MODULE "build calice module" ON
  "USER_CALICE" OFF)

include_directories(include)
aux_source_directory(src MODULE_SRC)

list(REMOVE_ITEM MODULE_SRC src/TLUConverterPlugin.cc)
list(REMOVE_ITEM MODULE_SRC src/NIConverterPlugin.cc)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${PROJECT_NAME})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
