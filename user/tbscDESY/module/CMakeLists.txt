include_directories(include)
#include_directories(/afs/desy.de/user/m/mengqing/kpix_dev/generic)
#include_directories(/afs/desy.de/user/m/mengqing/kpix_dev/kpix)
#include_directories(/afs/desy.de/user/m/mengqing/kpix_dev/deviceLib)
#include_directories(/usr/include/libxml2)

aux_source_directory(src MODULE_SRC)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
message("[dev]: project_source_dir is ${PROJECT_SOURCE_DIR}; eudaq_install_rpath is ${EUDAQ_INSTALL_RPATH}")
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} -L${PROJECT_SOURCE_DIR}/extern/lib/ -lodbc)

# Add to modules library:
TARGET_LINK_LIBRARIES(modules INTERFACE ${EUDAQ_MODULE})

install(TARGETS
  ${EUDAQ_MODULE}
  EXPORT eudaqTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
