find_package(ROOT OPTIONAL_COMPONENTS XrdCl)
find_package(XRootD CONFIG REQUIRED)

# Check if XRootD libraries and include directories are found
if (NOT XROOTD_CLIENT_LIBRARIES)
    message(WARNING "XRootD not found automatically. Make sure to specify the XRootD library directory manually if needed (-DXROOTD_LIBRARY_DIRS and -DXROOTD_INCLUDE_DIRS if necessary).")

    # Allow the user to specify the XRootD library directory manually
    set(XROOTD_LIBRARY_DIRS "" CACHE PATH "Path to the XRootD library directory (e.g., /path/to/xrootd/lib)")

    # Validate the provided library directory and find the library
    if (XROOTD_LIBRARY_DIRS)
        find_library(XROOTD_LIB XrdCl PATHS "${XROOTD_LIBRARY_DIRS}")
        if (NOT XROOTD_LIB)
            message(FATAL_ERROR "Invalid XRootD library directory: XrdCl library not found in ${XROOTD_LIBRARY_DIRS}.")
        endif()
        message(STATUS "Using manually provided XRootD library directory: ${XROOTD_LIBRARY_DIRS}")
        set(XROOTD_CLIENT_LIBRARIES "${XROOTD_LIB}")
    endif()

    if (NOT XROOTD_LIBRARY_DIRS)
        message(FATAL_ERROR "XRootD libraries not found! Please specify manually using -DXROOTD_LIBRARY_DIRS.")
    endif()
else()
    message(STATUS "Found XRootD libraries: ${XROOTD_CLIENT_LIBRARIES}")
endif()

# Add sources
aux_source_directory(src MODULE_SRC)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})

target_include_directories(${EUDAQ_MODULE} PRIVATE include ${XROOTD_INCLUDE_DIRS})

target_link_libraries(${EUDAQ_MODULE} PRIVATE ${EUDAQ_CORE_LIBRARY} ${XROOTD_CLIENT_LIBRARIES})

# Installation
install(
    TARGETS ${EUDAQ_MODULE}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
  