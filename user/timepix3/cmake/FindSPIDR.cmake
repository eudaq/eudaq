# - Try to find SPIDR

MESSAGE(STATUS "Looking for SPIDR...")

file(GLOB_RECURSE spidr_include $ENV{HOME}/SPIDR/software/SpidrTpx3Lib/SpidrController.h $ENV{SPIDRPATH}/software/SpidrTpx3Lib/SpidrController.h)
if(spidr_include)
    get_filename_component(spidr_lib_path "${spidr_include}" PATH)
    get_filename_component(spidr_lib_path "${spidr_lib_path}" PATH)
    set(SPIDR_ROOT ${spidr_lib_path} )
    MESSAGE(STATUS "Found SPIDR headers in ${SPIDR_ROOT}")
else(spidr_include)
    file(GLOB_RECURSE extern_file ${PROJECT_SOURCE_DIR}/extern/SPIDR/software/trunk/SpidrTpx3Lib/SpidrController.h)
       if (extern_file)
           # strip the file and 'include' path away:
    	   get_filename_component(extern_lib_path "${extern_file}" PATH)
	   get_filename_component(extern_lib_path "${extern_lib_path}" PATH)
	   MESSAGE(STATUS "Found SPIDR package in 'extern' subfolder: ${extern_lib_path}")
    	   set(SPIDR_ROOT ${extern_lib_path})
       endif(extern_file)
endif(spidr_include)

if (NOT SPIDR_ROOT)
   MESSAGE(ERROR " Could not find SPIDR package required by Timepix3 producer. Please copy 'https://svn.cern.ch/reps/SPIDR/software/trunk/SpidrTpx3Lib' either to $HOME/SPIDR or '../extern/SPIDR' directory")
endif()

set( SPIDR_INCLUDE ${SPIDR_ROOT}/SpidrTpx3Lib )
set( SPIDR_LIB_PATH ${SPIDR_ROOT}/Release )

mark_as_advanced(SPIDR_LIB SPIDR_ROOT )
