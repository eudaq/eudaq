LIST(APPEND MODULE_SRC src/CMSPhase2Event2StdEventConverter.cc)
MESSAGE(STATUS "CMS Phase2 StandardEvent converter will be built")

string(ASCII 27 Esc)
set(Reset    "${Esc}[m")
set(BoldRed  "${Esc}[1;31m")
set(BoldBlue "${Esc}[1;34m")

find_package(Boost REQUIRED CONFIG)
IF (Boost_FOUND)
  message("${BoldBlue}=== I found BOOST version ${Boost_VERSION} ===${Reset}")
ELSE (Boost_FOUND)
  message(FATAL_ERROR "${BoldRed}=== I didn't find any BOOST. BOOST is needed to compile the CMS Phase2 StandardEvent converter ===${Reset}")
ENDIF (Boost_FOUND)

ADD_LIBRARY(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
TARGET_INCLUDE_DIRECTORIES(${EUDAQ_MODULE} PRIVATE include)
TARGET_LINK_LIBRARIES(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} Boost::headers)

INSTALL(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
