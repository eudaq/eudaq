find_package(ZestSC1)
find_package(LIBUSB)
find_package(TLUFirmware)

cmake_dependent_option(USER_EUDET_HARDWARE "build telescope hardware lib" ON
  "USER_EUDET;LIBUSB_FOUND;ZestSC1_FOUND;TLUFirmware_FOUND" OFF)

IF(USER_EUDET AND (NOT USER_EUDET_HARDWARE))
  message("Telescope hardware lib is not to be built")
ENDIF()

IF(NOT USER_EUDET_HARDWARE)
  return()
ENDIF()

find_package(ZestSC1 REQUIRED)
find_package(LIBUSB REQUIRED)
find_package(TLUFirmware REQUIRED)

include_directories(include)
include_directories(${ZESTSC1_INCLUDE_DIRS})
include_directories(${LIBUSB_INCLUDE_DIRS})
add_definitions(${TLUFIRMWARE_DEFINITIONS})

if(USER_EUDET_HARDWARE)
  include_directories(include)
  
  SET(HARDWARE_SRC
    src/TLUController.cc
    src/TLUAddresses1.cc
    src/TLUAddresses2.cc
    src/NiController.cc
    )

  add_library(${USER_HARDWARE} STATIC ${HARDWARE_SRC})
  target_link_libraries(${USER_HARDWARE} ${EUDAQ_CORE} ${LIBUSB_LIBRARIES} ${ZESTSC1_LIBRARIES})
  
  install(TARGETS
    ${USER_HARDWARE}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

  if(WIN32 AND LIBUSB_FOUND AND LIBUSB_WIN_DLL)
    INSTALL(FILES
      ${LIBUSB_WIN_DLL}
      DESTINATION bin)
  endif()
endif()
