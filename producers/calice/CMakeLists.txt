option(BUILD_AHCAL "Compile AHCAL" OFF)
IF(BUILD_AHCAL)
  add_subdirectory(AHCAL)
ENDIF()

option(BUILD_BIFAHCAL "Compile BIFAHCAL" OFF)
IF(BUILD_BIFAHCAL)
  add_subdirectory(BIF_AHCAL)
ENDIF()

