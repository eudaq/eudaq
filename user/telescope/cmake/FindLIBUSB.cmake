# This CMake script wants to use libusb functionality, therefore it looks 
# for libusb include files and libraries. 
#
# Operating Systems Supported:
# - Unix (requires pkg-config)
#   Tested with Ubuntu 9.04 and Fedora 11
# - Windows (requires MSVC)
#   Tested with Windows XP
#
# This should work for both 32 bit and 64 bit systems.
#
# Original author: F. Kooman <fkooman@tuxed.net>
#

# FreeBSD has built-in libusb since 800069
IF(CMAKE_SYSTEM_NAME MATCHES FreeBSD)
  EXEC_PROGRAM(sysctl ARGS -n kern.osreldate OUTPUT_VARIABLE FREEBSD_VERSION)
  SET(MIN_FREEBSD_VERSION 800068)
  IF(FREEBSD_VERSION GREATER ${MIN_FREEBSD_VERSION})
    SET(LIBUSB_FOUND TRUE)
    SET(LIBUSB_INCLUDE_DIRS "/usr/include")
    SET(LIBUSB_LIBRARIES "usb")
    SET(LIBUSB_LIBRARY_DIRS "/usr/lib/")
  ENDIF(FREEBSD_VERSION GREATER ${MIN_FREEBSD_VERSION})
ENDIF(CMAKE_SYSTEM_NAME MATCHES FreeBSD)

IF(NOT LIBUSB_FOUND)
# find path of libusb installation in ./extern folder
file(GLOB_RECURSE extern_file ${CMAKE_CURRENT_LIST_DIR}/../extern/*usb.h)
if (extern_file)
    # strip the file and 'include' path away:
    get_filename_component(extern_lib_path "${extern_file}" PATH)
    get_filename_component(extern_lib_path "${extern_lib_path}" PATH)
    MESSAGE(STATUS "Found libusb package in 'extern' subfolder: ${extern_lib_path}")
endif(extern_file)

IF(MSVC)
  # for this version you need to have installed the lib usb driver to your system
  # Windows with Microsoft Visual C++ 
  FIND_PATH(LIBUSB_INCLUDE_DIRS 
    NAMES lusb0_usb.h
    PATHS "${extern_lib_path}/include" "$ENV{ProgramFiles}/LibUSB-Win32/include")
  if (${EX_PLATFORM} EQUAL 64)
    # on x64 (win64)
    FIND_LIBRARY(LIBUSB_LIBRARIES NAMES libusb PATHS "$ENV{ProgramFiles}/LibUSB-Win32/lib/msvc_x64" "${extern_lib_path}/lib/msvc_x64")
  ELSE(${EX_PLATFORM} EQUAL 64)
    # on x86 (win32)
    FIND_LIBRARY(LIBUSB_LIBRARIES NAMES libusb PATHS "$ENV{ProgramFiles}/LibUSB-Win32/lib/msvc" "${extern_lib_path}/lib/msvc")
  ENDIF(${EX_PLATFORM} EQUAL 64)
ELSE(MSVC)
  # If not MS Visual Studio we use PkgConfig
  FIND_PACKAGE (PkgConfig)
  IF(PKG_CONFIG_FOUND)
    PKG_CHECK_MODULES(LIBUSB REQUIRED libusb)
  ELSE(PKG_CONFIG_FOUND)
    MESSAGE(FATAL_ERROR "Could not find PkgConfig")
  ENDIF(PKG_CONFIG_FOUND)
ENDIF(MSVC)

IF(LIBUSB_INCLUDE_DIRS AND LIBUSB_LIBRARIES)
   SET(LIBUSB_FOUND TRUE)
ENDIF(LIBUSB_INCLUDE_DIRS AND LIBUSB_LIBRARIES)
ENDIF(NOT LIBUSB_FOUND)

IF(LIBUSB_FOUND)
  IF(NOT LIBUSB_FIND_QUIETLY)
    MESSAGE(STATUS "Found LIBUSB: ${LIBUSB_LIBRARIES} (lib) and ${LIBUSB_INCLUDE_DIRS} (header)")
  ENDIF (NOT LIBUSB_FIND_QUIETLY)
ELSE(LIBUSB_FOUND)
  IF(LIBUSB_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find LIBUSB")
  ENDIF(LIBUSB_FIND_REQUIRED)
ENDIF(LIBUSB_FOUND)
