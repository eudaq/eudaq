# Try to locate XRootD automatically
find_path(XROOTD_INCLUDE_DIR XrdCl/XrdClFile.hh)
find_library(XRDCL_LIB XrdCl)

# If XRootD is not found, allow the user to specify the directory manually
if (NOT XROOTD_INCLUDE_DIR OR NOT XRDCL_LIB)
    message(WARNING "XRootD not found automatically. Please specify the XRootD directory using -DXROOTD_DIRECTORY=<path>.")

    option(XROOTD_DIRECTORY "Path to the XRootD installation" "")

    if (XROOTD_DIRECTORY)
        # Set include and library paths from the user-defined option
        set(XROOTD_INCLUDE_DIR ${XROOTD_DIRECTORY})
        find_library(XRDCL_LIB XrdCl REQUIRED PATHS ${XROOTD_LIBRARY_DIR})

        if (NOT EXISTS ${XROOTD_INCLUDE_DIR}/XrdCl/XrdClFile.hh)
            message(FATAL_ERROR "Invalid XRootD directory: Header file not found in ${XROOTD_INCLUDE_DIR}.")
        endif()
    else()
        message(FATAL_ERROR "XRootD not found and no directory specified. Please set -DXROOTD_DIRECTORY=<path>.")
    endif()
else()
    message(STATUS "XRootD found automatically.")
endif()

# Add include directory and link library
include_directories(${XROOTD_INCLUDE_DIR})
link_directories(${XROOTD_LIBRARY_DIR})

# Add sources and library
include_directories(include)
aux_source_directory(src MODULE_SRC)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${XRDCL_LIB})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
