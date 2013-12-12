# - Try to find cactus

#macro(find_cactus_in_extern arg)
# find_path(CACTUS_ROOT cactuscore
#    HINTS ${PROJECT_SOURCE_DIR}/extern/cactus ${arg})
#endmacro()

#find_cactus_in_extern("")

set(CACTUS_ROOT ${PROJECT_SOURCE_DIR}/extern/cactus )

# could not find the package at the usual locations -- try to copy from AFS if accessible
if (NOT CACTUS_ROOT)
  MESSAGE(WARNING "Could not find CACTUS package required by miniTLU producer. Please refer to the documentation on how to obtain the software.")
endif()

set(CACTUS_BUILD_ROOT ${CACTUS_ROOT} )

set(EXTERN_ERLANG_PREFIX ${CACTUS_BUILD_ROOT}/cactuscore/extern/erlang/RPMBUILD/SOURCES )
set(EXTERN_ERLANG_BIN_PREFIX ${EXTERN_ERLANG_PREFIX}/bin )

set(EXTERN_BOOST_PREFIX ${CACTUS_BUILD_ROOT}/cactuscore/extern/boost/RPMBUILD/SOURCES )
set(EXTERN_BOOST_INCLUDE_PREFIX ${EXTERN_BOOST_PREFIX}/include )
set(EXTERN_BOOST_LIB_PREFIX ${EXTERN_BOOST_PREFIX}/lib )

set(EXTERN_PUGIXML_PREFIX ${CACTUS_BUILD_ROOT}/cactuscore/extern/pugixml/RPMBUILD/SOURCES )
set(EXTERN_PUGIXML_INCLUDE_PREFIX ${EXTERN_PUGIXML_PREFIX}/include )
set(EXTERN_PUGIXML_LIB_PREFIX ${EXTERN_PUGIXML_PREFIX}/lib )

set(UHAL_GRAMMARS_PREFIX ${CACTUS_BUILD_ROOT}/cactuscore/uhal/grammars/ )
set(UHAL_GRAMMARS_INCLUDE_PREFIX ${UHAL_GRAMMARS_PREFIX}/include )
set(UHAL_GRAMMARS_LIB_PREFIX ${UHAL_GRAMMARS_PREFIX}/lib )

set(UHAL_LOG_PREFIX ${CACTUS_BUILD_ROOT}/cactuscore/uhal/log/ )
set(UHAL_LOG_INCLUDE_PREFIX ${UHAL_LOG_PREFIX}/include )
set(UHAL_LOG_LIB_PREFIX ${UHAL_LOG_PREFIX}/lib )

set(UHAL_UHAL_PREFIX ${CACTUS_BUILD_ROOT}/cactuscore/uhal/uhal/ )
set(UHAL_UHAL_INCLUDE_PREFIX ${UHAL_UHAL_PREFIX}/include )
set(UHAL_UHAL_LIB_PREFIX ${UHAL_UHAL_PREFIX}/lib )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZESTSC1_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CACTUS  DEFAULT_MSG
                                  CACTUS_ROOT)

mark_as_advanced(CACTUS_LIB CACTUS_ROOT )
