# Determine platform- and compiler-specific settings

# demand c++11 support
set (CMAKE_CXX_STANDARD 11)
set_property (GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)

# platform dependent preprocessor defines
if (WIN32)
  if (CYGWIN)
    ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_CYGWIN")
  elseif (MINGW)
    ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_MINGW")
  else()
    ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_WIN32")
  endif()
elseif (UNIX)
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_MACOSX")
  else()
    ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_LINUX")
  endif()
else()
  MESSAGE(WARNING "WARNING: Platform not defined in CMakeLists.txt -- assuming Unix/Linux (good luck)." )
  ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_LINUX")
endif()

# determine compiler type (32/64bit)
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
#    MESSAGE(STATUS "64-bit compiler detected" )
    SET( EX_PLATFORM 64 )
    SET( EX_PLATFORM_NAME "x64" )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 ) 
#    MESSAGE(STATUS "32-bit compiler detected" )
    SET( EX_PLATFORM 32 )
    SET( EX_PLATFORM_NAME "x86" )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

# compiler specific settings
if (CMAKE_COMPILER_IS_GNUCC)
   # add some more general preprocessor defines (only for gcc)
   message(STATUS "Using gcc-specific preprocessor identifiers")
   add_definitions("-DEUDAQ_FUNC=__PRETTY_FUNCTION__ ")
   add_definitions("-D_GLIBCXX_USE_NANOSLEEP")

   LIST ( APPEND CMAKE_CXX_FLAGS "-fPIC" )
   LIST ( APPEND CMAKE_LD_FLAGS "-fPIC" )

elseif(MSVC)
   message(STATUS "Using MSVC-specific preprocessor identifiers and options")
   add_definitions("-DEUDAQ_FUNC=__FUNCTION__")
   add_definitions("/wd4251") # disables warning concerning dll-interface (comes up for std classes too often)
   add_definitions("/wd4996") # this compiler warnung is about that functions like fopen are unsafe.
   add_definitions("/wd4800") # disables warning concerning usage of old style bool (in root)
else()
   add_definitions("-DEUDAQ_FUNC=__func__")
endif()
