list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)

set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_TLU_BUILD "build tlu" OFF)
option(USER_BUILD_TLU_ONLY_CONVERTER "build only the TLU converters (TLU Prodcuers will not be built)" OFF)

# if neither producer, nor converters should be build we can return

if(NOT USER_TLU_BUILD AND NOT USER_BUILD_TLU_ONLY_CONVERTER)
    message(STATUS "user/tlu is NOT to be built (USER_TLU_BUILD=OFF)")
    message(STATUS "user/tlu CONVERTERS are NOT to be built (USER_BUILD_TLU_ONLY_CONVERTER=OFF)")
  unset(USER_TLU_BUILD_EUDET CACHE)
  unset(USER_TLU_BUILD_AIDA CACHE)
  return()
endif()

if(NOT USER_BUILD_TLU_ONLY_CONVERTER)
    message(STATUS "user/tlu is to be built (USER_TLU_BUILD=ON)")
else()
    message(STATUS "user/tlu is not to be built (flag overwritten by \"USER_BUILD_TLU_ONLY_CONVERTER=ON\")")
    message(STATUS "user/tlu CONVERTERS are to be built (USER_BUILD_TLU_ONLY_CONVERTER=ON)")
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})

# if we only want to built the converters we do not need to check for hardware libs
if(NOT USER_BUILD_TLU_ONLY_CONVERTER)
    # for the AIDA TLU
find_package(CACTUS)
cmake_dependent_option(USER_TLU_BUILD_AIDA "build user/eudet AIDA TLU" ON
  "CACTUS_FOUND" OFF)

# for the EUDET TLU
    find_package(ZestSC1)
    find_package(LIBUSB)
    find_package(TLUFirmware)
endif()
    cmake_dependent_option(USER_TLU_BUILD_EUDET "build user/eudet EUDET TLU" ON
  "LIBUSB_FOUND;ZESTSC1_FOUND;TLUFIRMWARE_FOUND" OFF)

if(USER_TLU_BUILD_AIDA OR USER_TLU_BUILD_EUDET)
    set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)
endif()


include_directories(hardware/include)
add_subdirectory(hardware)
add_subdirectory(module)
add_subdirectory(exe)
