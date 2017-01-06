list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_EUDET "build eudet" ON)

if(NOT USER_EUDET)
  return()
endif()

option(USER_EUDET_BUILD_MODULE "build user/eudet runtime module" ON)
set(USER_BUILD_MODULE ${USER_EUDET_BUILD_MODULE})

option(USER_EUDET_BUILD_NI "build user/eudet ni" ON)
set(USER_BUILD_NI ${USER_EUDET_BUILD_NI})

find_package(CACTUS)
cmake_dependent_option(USER_EUDET_BUILD_MINITLU "build user/eudet minitlu" ON
  "CACTUS_FOUND" OFF)
set(USER_BUILD_MINITLU ${USER_EUDET_BUILD_MINITLU})

find_package(ZestSC1)
find_package(LIBUSB)
find_package(TLUFirmware)
cmake_dependent_option(USER_EUDET_BUILD_TLU "build user/eudet tlu" ON
  "LIBUSB_FOUND;ZESTSC1_FOUND;TLUFIRMWARE_FOUND" OFF)
set(USER_BUILD_TLU ${USER_EUDET_BUILD_TLU})

include_directories(hardware/include)
add_subdirectory(hardware)
add_subdirectory(module)
add_subdirectory(exe)
