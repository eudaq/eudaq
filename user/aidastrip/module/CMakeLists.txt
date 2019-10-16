include_directories(include)
#include_directories(/home/lycoris-admin/kpixDaq/kpix_dev/generic)
#include_directories(/home/lycoris-admin/kpixDaq/kpix_dev/kpix)
#include_directories(/home/lycoris-admin/kpixDaq/kpix_dev/deviceLib)
include_directories(/usr/local/include/kpix)
include_directories(/usr/include/libxml2)

aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_LCIO_LIBRARY)
  message("[aidaStrip]: no EUDAQ_LCIO_LIBRARY, kpixRawEvt2LCEventConverter OFF!")
  list(REMOVE_ITEM MODULE_SRC src/kpixRawEvt2LCEventConverter.cc)
endif()


add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
#message("[dev]: project_source_dir is ${PROJECT_SOURCE_DIR}; eudaq_install_rpath is ${EUDAQ_INSTALL_RPATH}")
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY}
  ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES}
#  -L${PROJECT_SOURCE_DIR}/extern/lib/ -lkpix)
  -L/usr/local/lib/kpix/ -lkpix) 

##--- set run path of the dynamic kpix lib
set_target_properties(${EUDAQ_MODULE} PROPERTIES INSTALL_RPATH
  ${EUDAQ_INSTALL_RPATH}:/usr/local/lib/kpix/)

# Add to modules library:
target_link_libraries(modules INTERFACE ${EUDAQ_MODULE})


install(TARGETS
  ${EUDAQ_MODULE}
  EXPORT eudaqTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
