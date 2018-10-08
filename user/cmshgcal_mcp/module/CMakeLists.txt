option(CMSHGCAL_MCP_BUILD "build cmshgcal mcp producer" OFF)

if(NOT CMSHGCAL_MCP_BUILD)
  message("disable building of user/cmshgcal_dwc  (CMSHGCAL_MCP_BUILD=OFF)")

	include_directories(include ${EXTERN_BOOST_INCLUDE_PREFIX} ${ROOT_INCLUDE_DIR})
	LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${EXTERN_BOOST_LIB_PREFIX} )
	SET(MODULE_SRC src/CAEN_v17XX_Unpacker.cc)
	SET(CONVERTER_SRC src/CMSHGCal_MCP_RawEvent2StdEventConverter.cc)

	add_definitions(-DLINUX)		#necessary for compilation on LINUX
	add_definitions(-std=c++11)

	add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC} ${CONVERTER_SRC})
	target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} libCAENComm.so libCAENDigitizer.so libCAENVME.so)

	install(TARGETS
	  ${EUDAQ_MODULE}
	  RUNTIME DESTINATION bin
	  LIBRARY DESTINATION lib
	  ARCHIVE DESTINATION lib)

endif()


if(CMSHGCAL_MCP_BUILD)
  message("enabling building of user/cmshgcal_dwc  (CMSHGCAL_MCP_BUILD=ON)")
	
	include_directories(include ${EXTERN_BOOST_INCLUDE_PREFIX} ${ROOT_INCLUDE_DIR})
	LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${EXTERN_BOOST_LIB_PREFIX} )
	aux_source_directory(src MODULE_SRC)

	add_definitions(-DLINUX)		#necessary for compilation on LINUX
	add_definitions(-std=c++11)

	add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
	target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} libCAENComm.so libCAENDigitizer.so libCAENVME.so)

	install(TARGETS
	  ${EUDAQ_MODULE}
	  RUNTIME DESTINATION bin
	  LIBRARY DESTINATION lib
	  ARCHIVE DESTINATION lib)  
endif()