INCLUDE_DIRECTORIES(. include)

MESSAGE(STATUS "CMSIT converter plugin will be built")

# BOOST
string(ASCII 27 Esc)
set(Reset    "${Esc}[m")
set(BoldRed  "${Esc}[1;31m")
set(BoldBlue "${Esc}[1;34m")
find_package(Boost 1.70 REQUIRED CONFIG COMPONENTS serialization)
IF (Boost_FOUND)
  message("${BoldBlue}=== I found BOOST version ${Boost_VERSION} ===${Reset}")
# ROOT
find_package(ROOT QUIET)
IF (ROOT_FOUND)
  message("${BoldBlue}=== I found ROOT - compiling CMSIT converter plugin with ROOT support ===${Reset}")
ELSE (ROOT_FOUND)
  message("${BoldRed}=== I didn't find ROOT - compiling CMSIT converter plugin WITHOUT ROOT support ===${Reset}")
ENDIF(ROOT_FOUND)

if (ROOT_FOUND)
    set(CMSIT_WITH_ROOT_SUPPORT 1)
else(ROOT_FOUND)
    set(CMSIT_WITH_ROOT_SUPPORT 0)
endif(ROOT_FOUND)

set_source_files_properties(src/CMSITConverterPlugin.cc PROPERTIES GENERATED TRUE)
set_source_files_properties(include/CMSITConverterPlugin.hh PROPERTIES GENERATED TRUE)
configure_file(include/CMSITConverterPlugin.hh.in CMSITConverterPlugin.hh @ONLY)
configure_file(src/CMSITConverterPlugin.cc.in CMSITConverterPlugin.cc @ONLY)

ADD_LIBRARY(${EUDAQ_MODULE} SHARED ${CMAKE_CURRENT_BINARY_DIR}/CMSITConverterPlugin.cc)
set_target_properties(${EUDAQ_MODULE} PROPERTIES LINKER_LANGUAGE CXX)
TARGET_LINK_LIBRARIES(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} Boost::serialization)

IF (ROOT_FOUND)
  TARGET_LINK_LIBRARIES(${EUDAQ_MODULE} ROOT::Core ROOT::Hist)
ENDIF(ROOT_FOUND)

INSTALL(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
ELSE (Boost_FOUND)
  message("${BoldRed}=== I didn't find any BOOST. BOOST is needed to run with the InnerTracker DAQ ===${Reset}")
ENDIF (Boost_FOUND)

