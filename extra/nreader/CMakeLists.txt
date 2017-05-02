# CMake script to build library providing native reader Marlin processor for data conversion into LCIO
# and reconstruction using EUTelescope analysis framework
# build native reader used for data conversion into LCIO
option(EUDAQ_EXTRA_BUILD_NREADER "Compile native reader shared library (requires LCIO/EUTelescope)?" OFF)
IF(EUDAQ_EXTRA_BUILD_NREADER)
  SET(USE_LCIO ON)
  FIND_PACKAGE(EUTelescope REQUIRED)
  FIND_PACKAGE(LCIO REQUIRED)
  INCLUDE_DIRECTORIES(${LCIO_INCLUDE_DIRS} ${EUTELESCOPE_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/nreader/include)
  add_definitions(${LCIO_DEFINITIONS} ${EUTELESCOPE_DEFINITIONS})
  AUX_SOURCE_DIRECTORY( ${PROJECT_SOURCE_DIR}/nreader/detdescription detdescr_library_sources )
  ADD_LIBRARY( DetectorDescriptons SHARED ${detdescr_library_sources} )
  INSTALL(TARGETS DetectorDescriptons RUNTIME DESTINATION bin
                                      LIBRARY DESTINATION lib
                                      ARCHIVE DESTINATION lib)
  set(ADDITIONAL_LIBRARIES ${LCIO_LIBRARIES} ${EUTELESCOPE_LIBRARIES} DetectorDescriptons)

  SET( libname NativeReader )

  FIND_PACKAGE( ILCSOFTCMake REQUIRED)
  include (${ILCSOFT_CMAKE_PATH})

  FIND_PACKAGE( ILCUTIL COMPONENTS ILCSOFT_CMAKE_MODULES REQUIRED ILCTEST)

  # load default settings from ILCSOFT_CMAKE_MODULES
  INCLUDE( ilcsoft_default_settings )

  FIND_PACKAGE( Marlin 1.0 REQUIRED )
  FIND_PACKAGE( MarlinUtil 1.2 REQUIRED )

  LINK_LIBRARIES( ${EUDAQ_CORE_LIBRARY} )

  FOREACH( pkg Marlin MarlinUtil EUTELESCOPE)
    IF( ${pkg}_FOUND )
      # include as "system" libraries: gcc will be less verbose w.r.t. warnings
      INCLUDE_DIRECTORIES(SYSTEM ${${pkg}_INCLUDE_DIRS} )
      LINK_LIBRARIES( ${${pkg}_LIBRARIES} )
      ADD_DEFINITIONS ( ${${pkg}_DEFINITIONS} )
    ELSE()
      MESSAGE( STATUS "${pkg} not found" )
    ENDIF()
  ENDFOREACH()


  ### LIBRARY #################################################################

  ADD_DEFINITIONS("-DUSE_EUDAQ")
  ADD_DEFINITIONS("-DUSE_MARLIN")
  ADD_DEFINITIONS("-DUSE_GEAR")
  ADD_DEFINITIONS("-DUSE_MARLINUTIL")

  # ---------------------------------------------------------------------------

  # include directories
  INCLUDE_DIRECTORIES( ./include )

  AUX_SOURCE_DIRECTORY( ./src library_sources )
  ADD_LIBRARY( ${libname} SHARED ${library_sources} )

  INSTALL(TARGETS ${libname}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

ENDIF()
