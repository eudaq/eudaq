INCLUDE_DIRECTORIES(include)

LIST(APPEND MODULE_SRC src/CMSITConverterPlugin.cc)
MESSAGE(STATUS "CMSIT converter plugin will be built")

ADD_LIBRARY(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
TARGET_LINK_LIBRARIES(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_LCIO_LIBRARY} ${EXT_LIBRARIES})

# Boost
string(ASCII 27 Esc)
set(Reset    "${Esc}[m")
set(BoldRed  "${Esc}[1;31m")
set(BoldBlue "${Esc}[1;34m")
find_package(Boost 1.53 REQUIRED serialization)
include_directories(${Boost_INCLUDE_DIRS})
link_libraries(${Boost_SERIALIZATION_LIBRARY})
IF (Boost_FOUND)
  message("${BoldBlue}=== I found BOOST version ${Boost_VERSION} ===${Reset}")
ELSE (Boost_FOUND)
  message("${BoldRed}=== I didn't find any BOOST. BOOST is needed to run with the InnerTracker DAQ ===${Reset}")
  list(REMOVE_ITEM plugins_sources src/CMSITConverterPlugin.cc)
ENDIF (Boost_FOUND)

INSTALL(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
