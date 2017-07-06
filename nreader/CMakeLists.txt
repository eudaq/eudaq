# CMake script to build library providing native reader Marlin processor for data conversion into LCIO
# and reconstruction using EUTelescope analysis framework
SET( libname NativeReader )

FIND_PACKAGE( ILCSOFTCMake REQUIRED)
include (${ILCSOFT_CMAKE_PATH})

FIND_PACKAGE( ILCUTIL COMPONENTS ILCSOFT_CMAKE_MODULES REQUIRED ILCTEST)

# load default settings from ILCSOFT_CMAKE_MODULES
INCLUDE( ilcsoft_default_settings )

FIND_PACKAGE( Marlin 1.0 REQUIRED )

LINK_LIBRARIES( EUDAQ )

FOREACH( pkg Marlin EUTELESCOPE)
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

