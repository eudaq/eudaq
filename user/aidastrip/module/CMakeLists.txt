include_directories(include)
include_directories(/usr/include/libxml2)
include_directories(${KPIX_INCLUDE_DIR})
#message("[mengqing] aidastrip/module is ${KPIX_INCLUDE_DIR}")

aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_LCIO_LIBRARY)
  message("[aidaStrip]: no EUDAQ_LCIO_LIBRARY, kpixRawEvt2LCEventConverter OFF!")
  list(REMOVE_ITEM MODULE_SRC src/kpixRawEvt2LCEventConverter.cc)
endif()

if (USE_KPIX_NEW)
  list(REMOVE_ITEM MODULE_SRC src/kpixRawEvt2StdEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/kpixRawEvt2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/kpixProducer.cc)
else()
  list(REMOVE_ITEM MODULE_SRC src/kpixRawEvent2StdEventConverter.cc)
endif(USE_KPIX_NEW)

#unset(USE_KPIX_NEW CACHE)


#-- find necessary kpix libraries:
# find_library(MYLIB kpix /usr/local/lib/kpix)
# message("[dev] I found MYLIB is ${MYLIB}")
# set (KPIX_LIB_OLD /usr/local/lib/libkpix.so.1)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
#message("[dev]: project_source_dir is ${PROJECT_SOURCE_DIR}; eudaq_install_rpath is ${EUDAQ_INSTALL_RPATH}")

target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY}
  ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES}
#  -L${PROJECT_SOURCE_DIR}/extern/lib/ -lkpix)
  -L${KPIX_LIB_DIR} -lkpix) 

##--- set run path of the dynamic kpix lib
set_target_properties(${EUDAQ_MODULE} PROPERTIES INSTALL_RPATH
  ${EUDAQ_INSTALL_RPATH}:${KPIX_LIB_DIR})


install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

