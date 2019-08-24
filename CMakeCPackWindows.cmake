# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "EUDAQ is distributed data acquisition sysem")
set(CPACK_PACKAGE_VENDOR "EUDAQ collaboration")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${EUDAQ_LIB_VERSION_STRIPPED}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "eudaq-${EUDAQ_LIB_VERSION_STRIPPED}")
# https://stackoverflow.com/questions/28768417/how-to-set-an-icon-in-nsis-install-cmake
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/images\\\\eudaq_logo.bmp")

option(CMAKE_INSTALL_DEBUG_LIBRARIES
  "Install Microsoft runtime debug libraries with CMake." FALSE)
mark_as_advanced(CMAKE_INSTALL_DEBUG_LIBRARIES)

# By default, do not warn when built on machines using only VS Express:
if(NOT DEFINED CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS ON)
endif()

if(CMake_INSTALL_DEPENDENCIES)
  #include(${CMake_SOURCE_DIR}/Modules/InstallRequiredSystemLibraries.cmake)
  include(InstallRequiredSystemLibraries.cmake)
endif()

# Installers for 32- vs. 64-bit CMake:
#  - Root install directory (displayed to end user at installer-run time)
#  - "NSIS package/display name" (text used in the installer GUI)
#  - Registry key used to store info about the installation
if(CMAKE_CL_64)
  set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
  set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION} (Win64)")
else()
  set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
  set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
endif()
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_NSIS_PACKAGE_NAME}")

if(NOT DEFINED CPACK_SYSTEM_NAME)
  # make sure package is not Cygwin-unknown, for Cygwin just
  # cygwin is good for the system name
  if("x${CMAKE_SYSTEM_NAME}" STREQUAL "xCYGWIN")
    set(CPACK_SYSTEM_NAME Cygwin)
  else()
    set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
  endif()
endif()
if(${CPACK_SYSTEM_NAME} MATCHES Windows)
  if(CMAKE_CL_64)
    set(CPACK_SYSTEM_NAME win64-x64)
    set(CPACK_IFW_TARGET_DIRECTORY "@RootDir@/Program Files/${CMAKE_PROJECT_NAME}")
  else()
    set(CPACK_SYSTEM_NAME win32-x86)
  endif()
endif()


if(NOT DEFINED CPACK_PACKAGE_FILE_NAME)
  # if the CPACK_PACKAGE_FILE_NAME is not defined by the cache
  # default to source package - system, on cygwin system is not
  # needed
  if(CYGWIN)
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}")
  else()
    set(CPACK_PACKAGE_FILE_NAME
      "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
  endif()
endif()

set(CPACK_PACKAGE_CONTACT "EUDAQ collaboration <github.com/eudaq/eudaq>")

if(UNIX)
  #set(CPACK_STRIP_FILES "${CMAKE_BIN_DIR}/ccmake;${CMAKE_BIN_DIR}/cmake;${CMAKE_BIN_DIR}/cpack;${CMAKE_BIN_DIR}/ctest")
  #set(CPACK_SOURCE_STRIP_FILES "")
  #set(CPACK_PACKAGE_EXECUTABLES "ccmake" "CMake")
  #set (CPACK_PACKAGE_INSTALL_DIRECTORY "eudaq")
  #execute_process(COMMAND ln -s /opt/eudaq/bin/TestReader.exe TestReader.exe)
  #install(FILES ${CMAKE_BINARY_DIR}/TestReader.exe DESTINATION /usr/bin)  
  file(GLOB files "${CMAKE_BINARY_DIR}/*")
  #foreach(exec_file_name ${files})
  #  execute_process(COMMAND ln -s /opt/eudaq/bin/${exec_file_name} ${exec_file_name})
  #  install(FILES ${CMAKE_BINARY_DIR}/${exec_file_name} DESTINATION /usr/bin) 
  #endforeach(exec_file_name)  
  #set(CPACK_INSTALL_PREFIX "/opt")s
endif()

set(CPACK_WIX_UPGRADE_GUID "8ffd1d72-b7f1-11e2-8ee5-00238bca4991")

if(MSVC AND NOT "$ENV{WIX}" STREQUAL "")
  set(WIX_CUSTOM_ACTION_ENABLED TRUE)
  if(CMAKE_CONFIGURATION_TYPES)
    set(WIX_CUSTOM_ACTION_MULTI_CONFIG TRUE)
  else()
    set(WIX_CUSTOM_ACTION_MULTI_CONFIG FALSE)
  endif()
else()
  set(WIX_CUSTOM_ACTION_ENABLED FALSE)
endif()

# Set the options file that needs to be included inside CMakeCPackOptions.cmake
set(QT_DIALOG_CPACK_OPTIONS_FILE ${CMake_BINARY_DIR}/Source/QtDialog/QtDialogCPack.cmake)

configure_file("CMakeCPackOptionsWindows.cmake.in" "CMakeCPackOptionsWindows.cmake" @ONLY)
set(CPACK_PROJECT_CONFIG_FILE "CMakeCPackOptionsWindows.cmake")

#set(CPACK_COMPONENTS_ALL MAIN_LIB MAIN_EXE GUI TLU NI ONLINEMON MAIN_PYTHON RPICONTROLLER OFFLINEMON)

# Either set generator here or call by cpack -G $GeneratorName from shell
# SET(CPACK_GENERATOR "DEB")

# include CPack model once all variables are set
include(CPack)
