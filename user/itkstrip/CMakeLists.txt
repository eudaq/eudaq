get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_ITKSTRIP_BUILD "build user/itkstrip" OFF)

if(NOT USER_ITKSTRIP_BUILD)
  message(STATUS "user/itkstrip is NOT to be built (USER_ITKSTRIP_BUILD=OFF)")
  return()
else()
    message(STATUS "user/itkstrip is to be built (USER_ITKSTRIP_BUILD=ON)")
endif()


add_subdirectory(module)
add_subdirectory(exe)

set(CMAKE_PREFIX_PATH $ENV{ROOTSYS})
set(ROOT_DIR $ENV{ROOTSYS}/cmake)

if(WIN32)
  find_package(ROOT REQUIRED COMPONENTS libCore libCint)
else()
  find_package(ROOT)
endif()

cmake_dependent_option(USER_ITKSTRIP_BUILD_ITSROOT "build ITSDAQ ROOT Producer" ON
  "ROOT_FOUND" OFF)
if(USER_ITKSTRIP_BUILD_ITSROOT)
  add_subdirectory(itsroot)
endif()
