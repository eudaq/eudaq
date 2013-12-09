# - Try to find pthreads-w32 compiled library and includes
# Once done this will define
#  W32PTHREADS_FOUND - System has pthreads-w32
#  W32PTHREADS_INCLUDE_DIRS - The pthreads-w32 include directories
#  W32PTHREADS_LIBRARIES - The libraries needed to use pthreads-w32
#  W32PTHREADS_DEFINITIONS - Compiler switches required for using pthreads-w32

  find_path(W32PTHREADS_INCLUDE_DIR pthread.h
    HINTS 
    "${W32PTHREADS}/include" 
    "$ENV{W32PTHREADS}/include"
    "c:/pthreads-w32/include"
    "${PROJECT_SOURCE_DIR}/extern/pthreads-w32/include"
    )

if (${EX_PLATFORM} EQUAL 64)
  if(MSVC)
    SET(libname  pthreadVC2)
  else() # gcc
    SET(libname  libpthreadGC2.a)
  endif(MSVC)
else() #32bit
  if(MSVC)
    SET(libname  pthreadVC2 )
  else() # gcc
    SET(libname  libpthreadGC2.a )
  endif(MSVC)
endif(${EX_PLATFORM} EQUAL 64)

find_library(W32PTHREADS_LIBRARY NAMES ${libname}
  HINTS 
  "${W32PTHREADS}/lib/${EX_PLATFORM_NAME}" 
  "$ENV{W32PTHREADS}/lib/${EX_PLATFORM_NAME}"
  "c:/pthreads-w32/lib/${EX_PLATFORM_NAME}"
  "${PROJECT_SOURCE_DIR}/extern/pthreads-w32/lib/${EX_PLATFORM_NAME}"
  )

set(W32PTHREADS_LIBRARIES ${W32PTHREADS_LIBRARY} )
set(W32PTHREADS_INCLUDE_DIRS ${W32PTHREADS_INCLUDE_DIR} )
set(W32PTHREADS_DEFINITIONS "-DUSE_W32PTHREADS" )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set W32PTHREADS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(pthreads-w32  DEFAULT_MSG
                                  W32PTHREADS_LIBRARY W32PTHREADS_INCLUDE_DIR)

mark_as_advanced(W32PTHREADS_INCLUDE_DIR W32PTHREADS_LIBRARY )