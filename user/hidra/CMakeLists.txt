##-- OPT: Compile HIDRA?
option(USER_HIDRA_BUILD "Build HiDRa DAQ system" OFF)

##-- OPT: Compile each module?
option(USER_HIDRA_FERS_BUILD "Build HiDRa FERS component" OFF)
option(USER_HIDRA_XDC_BUILD "Build HiDRa XDC component" ON)
option(USER_HIDRA_DRY_BUILD "Build HiDRa XDC component" OFF)
option(USER_HIDRA_DAQ_BUILD "Build HiDRa DAQ component" ON)

if(NOT USER_HIDRA_BUILD)
	message(STATUS "user/hidra is NOT to be built (USER_HIDRA_BUILD=OFF)")
	return()
endif()
message(STATUS "user/hidra is to be built (USER_HIDRA_BUILD=ON)")


##-- Global Preparation:
#-- EUDAQ_MODULE needed for compilation at lower level CMakeLists
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE eudaq_module_${EUDAQ_USERNAME})

include_directories(${EUDAQ_INCLUDE_DIRS})
aux_source_directory(rc/src MODULE_SRC)
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY} ${EUDAQ_ROOT_MONITOR_LIBRARY})

##-- DAQ Components
#-- Add component folder
if(USER_HIDRA_FERS_BUILD)
	message(STATUS "user/hidra FERS component is to be built (USER_HIDRA_FERS_BUILD=ON)")

	add_subdirectory(fers) 
	target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_MODULE}_fers)
else()
	message(STATUS "user/hidra FERS component is NOT to be built (USER_HIDRA_FERS_BUILD=OFF)")
endif()

if(USER_HIDRA_XDC_BUILD)
	message(STATUS "user/hidra XDC component is to be built (USER_HIDRA_XDC_BUILD=ON)")

        add_subdirectory(xdc)
	target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_MODULE}_xdc)
else()
	message(STATUS "user/hidra XDC component is NOT to be built (USER_HIDRA_XDC_BUILD=OFF)")
endif()


if(USER_HIDRA_DRY_BUILD)
	message(STATUS "user/hidra dry-run component is to be built (USER_HIDRA_DRY_BUILD=ON)")

        add_subdirectory(dry)
	target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_MODULE}_dry)
else()
	message(STATUS "user/hidra dry-run component is NOT to be built (USER_HIDRA_DRY_BUILD=OFF)")
endif()


if(USER_HIDRA_DAQ_BUILD)
	message(STATUS "user/hidra DAQ component is to be built (USER_HIDRA_DAQ_BUILD=ON)")
	add_subdirectory(dc)     
	target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_MODULE}_dc)
else()
	message(STATUS "user/hidra DAQ component is NOT to be built (USER_HIDRA_DAQ_BUILD=OFF)")
endif()

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

