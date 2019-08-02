##-- add include directories:
include_directories(include)
include_directories( ${LIBXML2_INCLUDE_DIR} )

##-- add all cxx files from source directory:
aux_source_directory(src MODULE_SRC)

##- remove files according to various dependancy environment
if(NOT EUDAQ_LCIO_LIBRARY)
  message("[aidaStrip]: no EUDAQ_LCIO_LIBRARY, kpixRawEvt2LCEventConverter OFF!")
  list(REMOVE_ITEM MODULE_SRC src/kpixRawEvt2LCEventConverter.cc)
endif()

##-- according to different kpix external lib, compile differently:

if (USER_AIDASTRIP_KPIX_OLD)
  list(REMOVE_ITEM MODULE_SRC src/kpixRawEvent2StdEventConverter.cc)
  set(KPIX_INCLUDE_DIR /usr/local/include/kpixSw_3.10)
  set(KPIX_LIB_DIR /usr/local/lib/kpixSw_3.10)
  include_directories(${KPIX_INCLUDE_DIR})

  add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
  target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY}
  ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES}
  -L${KPIX_LIB_DIR} -lkpix
  ${LIBXML2_LIBRARIES}
  )

  ##--- set run path of the dynamic kpix lib
  set_target_properties(${EUDAQ_MODULE}
	PROPERTIES INSTALL_RPATH ${EUDAQ_INSTALL_RPATH}:${KPIX_LIB_DIR})
  
else()
  list(REMOVE_ITEM MODULE_SRC src/kpixProducer.cc)

  add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
  target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY}
	${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES}
	${KPIX_LIBRARY} # this handles the IMPORTED targets auto and lets embed compiler flags, include paths, library dependencies
	${LIBXML2_LIBRARIES}
	)
  
  ##--- set run path of the dynamic kpix lib
  set_target_properties(${EUDAQ_MODULE}
	PROPERTIES INSTALL_RPATH ${EUDAQ_INSTALL_RPATH})
  
endif(USER_AIDASTRIP_KPIX_OLD)

#add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
#message("[dev]: project_source_dir is ${PROJECT_SOURCE_DIR}; eudaq_install_rpath is ${EUDAQ_INSTALL_RPATH}")

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
