##-- OPT: Compile HIDRA?
option(USER_HIDRA_BUILD "Build HiDRa DAQ system" OFF)

##-- OPT: Compile each module?
option(USER_HIDRA_FERS_BUILD "Build HiDRa FERS component" OFF)
option(USER_HIDRA_FERS2_BUILD "Build HiDRa FERS2 component" ON)
option(USER_HIDRA_XDC_BUILD "Build HiDRa XDC component" ON)
option(USER_HIDRA_TRACKER_BUILD "Build HiDRa Tracker component" ON)
option(USER_HIDRA_DRY_BUILD "Build HiDRa Dry component" ON)
option(USER_HIDRA_DAQ_BUILD "Build HiDRa DAQ component" ON)
option(USER_HIDRA_MONITOR_BUILD "Build HiDRa Monitor component" ON)

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

add_subdirectory(common)

aux_source_directory(rc/src MODULE_SRC)
list(REMOVE_ITEM MODULE_SRC
	rc/src/HidraLogCollectorGUI.cc
	rc/src/hidraLog.cxx)
list(REMOVE_DUPLICATES MODULE_SRC)
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY} ${EUDAQ_ROOT_MONITOR_LIBRARY} ${EUDAQ_MODULE}_common)

if(EUDAQ_BUILD_GUI)
	if(NOT EUDAQ_FORCE_QT5)
		find_package(Qt6 COMPONENTS Widgets QUIET)
	endif()

	if((NOT Qt6_FOUND) OR (NOT DEFINED Qt6_FOUND))
		find_package(Qt5 5.15 COMPONENTS Widgets QUIET)
	endif()

	if(Qt6_FOUND OR Qt5_FOUND)
		set(CMAKE_AUTOUIC YES)
		set(CMAKE_AUTORCC YES)
		set(CMAKE_INCLUDE_CURRENT_DIR YES)

		qt_wrap_cpp(HIDRA_LOG_MOCSrcs
			${CMAKE_SOURCE_DIR}/gui/include/LogCollectorModel.hh
			${CMAKE_SOURCE_DIR}/gui/include/euLog.hh)
		qt_wrap_ui(HIDRA_LOG_UIrcs
			${CMAKE_SOURCE_DIR}/gui/ui/euLog.ui
			${CMAKE_SOURCE_DIR}/gui/ui/LogDialog.ui)

		add_executable(hidraLog
			${HIDRA_LOG_MOCSrcs}
			${HIDRA_LOG_UIrcs}
			rc/src/hidraLog.cxx
			rc/src/HidraLogCollectorGUI.cc
			${CMAKE_SOURCE_DIR}/gui/src/euLog.cc
			${CMAKE_SOURCE_DIR}/gui/src/LogCollectorModel.cc
			${CMAKE_SOURCE_DIR}/gui/images/euLog.qrc)

		target_include_directories(hidraLog PRIVATE
			rc/src
			${CMAKE_SOURCE_DIR}/gui/include)

		if(Qt6_FOUND)
			target_link_libraries(hidraLog ${EUDAQ_CORE_LIBRARY} Qt6::Widgets)
		else()
			target_link_libraries(hidraLog ${EUDAQ_CORE_LIBRARY} Qt5::Widgets)
		endif()

		install(TARGETS hidraLog
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib)
	else()
		message(STATUS "HiDRa GUI log collector is NOT built because Qt was not found")
	endif()
endif()

##-- DAQ Components
#-- Add component folder
if(USER_HIDRA_FERS_BUILD)
	message(STATUS "user/hidra FERS component is to be built (USER_HIDRA_FERS_BUILD=ON)")

	add_subdirectory(fers) 
	target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_MODULE}_fers)
else()
	message(STATUS "user/hidra FERS component is NOT to be built (USER_HIDRA_FERS_BUILD=OFF)")
endif()

if(USER_HIDRA_FERS2_BUILD)
	find_library(CAENFERSLIB_LIBRARY NAMES caenferslib)
	find_path(CAENFERSLIB_INCLUDE_DIR NAMES FERSlib.h)
	if(NOT CAENFERSLIB_LIBRARY OR NOT CAENFERSLIB_INCLUDE_DIR)
		message(WARNING "USER_HIDRA_FERS2_BUILD=ON but the CAEN FERS library and/or header FERSlib.h were not found — disabling FERS2 component. Install the CAENFERSlib development files or set USER_HIDRA_FERS2_BUILD=OFF to suppress this warning.")
		set(USER_HIDRA_FERS2_BUILD OFF CACHE BOOL "Build HiDRa FERS2 component" FORCE)
	else()
		set(CAENFERSLIB_LIBRARY "${CAENFERSLIB_LIBRARY}" CACHE FILEPATH "Resolved path to the CAEN FERS library" FORCE)
		set(CAENFERSLIB_INCLUDE_DIR "${CAENFERSLIB_INCLUDE_DIR}" CACHE PATH "Resolved path to the CAEN FERS headers" FORCE)
	endif()
endif()

if(USER_HIDRA_FERS2_BUILD)
	message(STATUS "user/hidra FERS2 component is to be built (USER_HIDRA_FERS2_BUILD=ON)")

	add_subdirectory(fers2)
	target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_MODULE}_fers2)
else()
	message(STATUS "user/hidra FERS2 component is NOT to be built (USER_HIDRA_FERS2_BUILD=OFF)")
endif()

if(USER_HIDRA_XDC_BUILD)
	find_library(CAENVME_LIBRARY NAMES CAENVME)
	find_library(CAENCOMM_LIBRARY NAMES CAENComm)
	find_path(CAENVME_HEADER_FOUND NAMES CAENVMElib.h)
	find_path(CAENVME_TYPES_HEADER_FOUND NAMES CAENVMEtypes.h)
	if(NOT CAENVME_LIBRARY OR NOT CAENCOMM_LIBRARY OR NOT CAENVME_HEADER_FOUND OR NOT CAENVME_TYPES_HEADER_FOUND)
		message(WARNING "USER_HIDRA_XDC_BUILD=ON but required CAEN VME libraries and headers were not found — disabling XDC component. Install CAENVMELib >= 4.1.3 development files (including CAENVMElib.h and CAENVMEtypes.h) and CAENComm >= 1.8.0, or set USER_HIDRA_XDC_BUILD=OFF to suppress this warning.")
		set(USER_HIDRA_XDC_BUILD OFF CACHE BOOL "Build HiDRa XDC component" FORCE)
	else()
		set(CAENVME_LIBRARY "${CAENVME_LIBRARY}" CACHE FILEPATH "Resolved path to the CAEN VME library" FORCE)
		set(CAENCOMM_LIBRARY "${CAENCOMM_LIBRARY}" CACHE FILEPATH "Resolved path to the CAEN Comm library" FORCE)
		set(CAENVME_HEADER_FOUND "${CAENVME_HEADER_FOUND}" CACHE PATH "Resolved path to the CAEN VME headers" FORCE)
		set(CAENVME_TYPES_HEADER_FOUND "${CAENVME_TYPES_HEADER_FOUND}" CACHE PATH "Resolved path to the CAEN VME types headers" FORCE)
	endif()
endif()

if(USER_HIDRA_XDC_BUILD)
	message(STATUS "user/hidra XDC component is to be built (USER_HIDRA_XDC_BUILD=ON)")

        add_subdirectory(xdc)
	target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_MODULE}_xdc)
else()
	message(STATUS "user/hidra XDC component is NOT to be built (USER_HIDRA_XDC_BUILD=OFF)")
endif()

if(USER_HIDRA_TRACKER_BUILD)
	message(STATUS "user/hidra Tracker component is to be built (USER_HIDRA_TRACKER_BUILD=ON)")

	add_subdirectory(tracker)
	target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_MODULE}_tracker)
else()
	message(STATUS "user/hidra Tracker component is NOT to be built (USER_HIDRA_TRACKER_BUILD=OFF)")
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

if(USER_HIDRA_MONITOR_BUILD)
	message(STATUS "user/hidra Monitor component is to be built (USER_HIDRA_MONITOR_BUILD=ON)")
	add_subdirectory(monitor)     
	target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_MODULE}_monitor)
else()
	message(STATUS "user/hidra Monitor component is NOT to be built (USER_HIDRA_MONITOR_BUILD=OFF)")
endif()

# HiDRA requires at least C++17: the monitor, dc and common use
# std::shared_mutex, std::string_view and std::filesystem, which are not
# available under C++14. Express this as a per-target minimum compile feature
# rather than a directory-wide CMAKE_CXX_STANDARD, so the requirement stays
# localized to HiDRA's own targets and propagates to anything linking them.
# cxx_std_17 only raises the floor — a higher standard selected upstream for
# ROOT (C++20/23 in cmake/Platform.cmake) is kept. The if(TARGET) guards skip
# components whose USER_HIDRA_*_BUILD option is OFF.
foreach(_hidra_target
		${EUDAQ_MODULE}
		${EUDAQ_MODULE}_common
		${EUDAQ_MODULE}_fers
		${EUDAQ_MODULE}_fers2
		${EUDAQ_MODULE}_xdc
		${EUDAQ_MODULE}_tracker
		${EUDAQ_MODULE}_dry
		${EUDAQ_MODULE}_dc
		${EUDAQ_MODULE}_monitor
		hidraLog)
	if(TARGET ${_hidra_target})
		target_compile_features(${_hidra_target} PUBLIC cxx_std_17)
	endif()
endforeach()

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
