find_package(ZestSC1)
find_package(LIBUSB)
find_package(TLUFirmware)

cmake_dependent_option(USER_TELESCOPE_EXE "build telescope executable" ON
  "USER_TELESCOPE;LIBUSB_FOUND;ZestSC1_FOUND;TLUFirmware_FOUND" OFF)

IF(USER_TELESCOPE AND (NOT USER_TELESCOPE_EXE))
  message("Telescope executables are not to be built")
ENDIF()

IF(NOT USER_TELESCOPE_EXE)
  return()
ENDIF()

find_package(ZestSC1 REQUIRED)
find_package(LIBUSB REQUIRED)
find_package(TLUFirmware REQUIRED)

include_directories(${ZESTSC1_INCLUDE_DIRS})
include_directories(${LIBUSB_INCLUDE_DIRS})
add_definitions(${TLUFIRMWARE_DEFINITIONS})

add_executable(TLUProducer src/TLUProducer.cxx)
add_executable(TLUControl src/TLUControl.cxx)
add_executable(TLUReset src/TLUReset.cxx)
add_executable(NiProducer src/NiProducer.cxx)

target_link_libraries(TLUProducer ${EUDAQ_CORE} ${USER_SHARED})
target_link_libraries(TLUControl ${EUDAQ_CORE} ${USER_SHARED})
target_link_libraries(TLUReset ${EUDAQ_CORE} ${USER_SHARED})
target_link_libraries(NiProducer ${EUDAQ_CORE} ${USER_SHARED})


add_executable(TelTest src/TelTest.cxx)
target_link_libraries(TelTest ${EUDAQ_CORE} ${USER_SHARED})
install(TARGETS
  TelTest
  RUNTIME DESTINATION bin)

install(TARGETS
  TLUProducer TLUControl TLUReset NiProducer
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  )
