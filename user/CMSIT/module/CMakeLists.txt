INCLUDE_DIRECTORIES(include)

LIST(APPEND MODULE_SRC src/CMSITConverterPlugin.cc)
MESSAGE(STATUS "CMSIT converter plugin will be built")

# BOOST
string(ASCII 27 Esc)
set(Reset    "${Esc}[m")
set(BoldRed  "${Esc}[1;31m")
set(BoldBlue "${Esc}[1;34m")
find_package(Boost 1.53 REQUIRED serialization)
include_directories(${Boost_INCLUDE_DIRS})
IF (Boost_FOUND)
  message("${BoldBlue}=== I found BOOST version ${Boost_VERSION} ===${Reset}")
ELSE (Boost_FOUND)
  message("${BoldRed}=== I didn't find any BOOST. BOOST is needed to run with the InnerTracker DAQ ===${Reset}")
  list(REMOVE_ITEM plugins_sources src/CMSITConverterPlugin.cc)
ENDIF (Boost_FOUND)

# ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT QUIET)
include_directories(${ROOT_INCLUDE_DIR})
IF (ROOT_USE_FILE)
  message("${BoldBlue}=== Including ROOT files ===${Reset}")
  include(${ROOT_USE_FILE})
ELSE (ROOT_USE_FILE)
  message("${BoldRed}=== I didn't find ROOT ===${Reset}")
ENDIF(ROOT_USE_FILE)
cmake_dependent_option(USER_CMSIT_BUILD "src/CMSITConverterPlugin (requires ROOT)" OFF "ROOT_FOUND" OFF)

ADD_LIBRARY(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
TARGET_LINK_LIBRARIES(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EXT_LIBRARIES} ${Boost_SERIALIZATION_LIBRARY} ${ROOT_LIBRARIES})

INSTALL(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
