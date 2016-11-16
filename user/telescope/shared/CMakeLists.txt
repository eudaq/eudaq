find_package(ZestSC1)
find_package(LIBUSB)
find_package(TLUFirmware)

cmake_dependent_option(USER_TELESCOPE_SHARED "build telescope hardware lib" ON
  "USER_TELESCOPE;LIBUSB_FOUND;ZestSC1_FOUND;TLUFirmware_FOUND" OFF)

IF(USER_TELESCOPE AND (NOT USER_TELESCOPE_SHARED))
  message("Telescope hardware lib is not to be built")
ENDIF()

IF(NOT USER_TELESCOPE_SHARED)
  return()
ENDIF()

find_package(ZestSC1 REQUIRED)
find_package(LIBUSB REQUIRED)
find_package(TLUFirmware REQUIRED)

include_directories(include)
include_directories(${ZESTSC1_INCLUDE_DIRS})
include_directories(${LIBUSB_INCLUDE_DIRS})
add_definitions(${TLUFIRMWARE_DEFINITIONS})

IF(USER_TELESCOPE_SHARED)
  include_directories(include)
  
  SET(SHARED_SRC
    src/TLUController.cc
    src/TLUAddresses1.cc
    src/TLUAddresses2.cc
    src/NiController.cc
    )

  add_library(${USER_SHARED} STATIC ${SHARED_SRC})
  target_link_libraries(${USER_SHARED} ${EUDAQ_CORE} ${LIBUSB_LIBRARIES} ${ZESTSC1_LIBRARIES})
  
  install(TARGETS
    ${USER_SHARED}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF()
