if(UNIX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBUSB REQUIRED libusb)
  if(LIBUSB_FOUND)
    message(STATUS "FOUND libusb-0.1")
    return()
  endif()
else()
  file(GLOB_RECURSE LIBUSB_HEADER
    ${CMAKE_CURRENT_LIST_DIR}/../extern/*/include/lusb0_usb.h)
  if(LIBUSB_HEADER)
    get_filename_component(LIBUSB_INCLUDE_DIRS ${LIBUSB_HEADER} PATH)
    message(STATUS "Found libusb-0.1 header: ${LIBUSB_HEADER}")
  else()
    message(STATUS "NOT Found libusb-0.1 at ${CMAKE_CURRENT_LIST_DIR}/../extern")
    message(STATUS "libusb-0.1 can be download from  https://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/1.2.6.0/")
    return()
  endif()
  if(${EX_PLATFORM} EQUAL 64)
    find_library(LIBUSB_LIBRARIES NAMES libusb
      PATHS ${LIBUSB_INCLUDE_DIRS}/../lib/msvc_x64)
    find_file(LIBUSB_WIN_DLL NAMES libusb0.dll
      PATHS ${LIBUSB_INCLUDE_DIRS}/../bin/amd64)
  else()
    find_library(LIBUSB_LIBRARIES NAMES libusb
      PATHS ${LIBUSB_INCLUDE_DIRS}/../lib/msvc)
    find_file(LIBUSB_WIN_DLL NAMES libusb0_x86.dll
      PATHS ${LIBUSB_INCLUDE_DIRS}/../bin/x86)
  endif()

  if((LIBUSB_INCLUDE_DIRS) AND (LIBUSB_LIBRARIES) AND (LIBUSB_WIN_DLL))
    set(LIBUSB_FOUND TRUE)
  endif()

endif()


if(LIBUSB_FOUND)
  if(NOT LIBUSB_FIND_QUIETLY)
    message(STATUS "Found libusb-0.1: ${LIBUSB_LIBRARIES} (lib) and ${LIBUSB_INCLUDE_DIRS} (header)")
  endif()
else()
  if(LIBUSB_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find LIBUSB")
  endif()
endif()
