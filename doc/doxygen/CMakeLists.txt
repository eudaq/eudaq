#############################################
# Doxygen target to generate HTML reference #
#############################################
OPTION(EUDAQ_BUILD_DOXYGEN "doc/doxygen code documentation (requires doxygen)" OFF)
if(NOT EUDAQ_BUILD_DOXYGEN)
  message(STATUS "doc/doxygen is NOT to be built (EUDAQ_BUILD_DOXYGEN=OFF)")
  return()
endif()
message(STATUS "doc/doxygen will be built (EUDAQ_BUILD_DOXYGEN=ON), if doxygen is found.")

FIND_PACKAGE(Doxygen)

IF(DOXYGEN_FOUND)
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  ADD_CUSTOM_TARGET(doxygen
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating HTML documentation with Doxygen" VERBATIM
    )
ENDIF()
