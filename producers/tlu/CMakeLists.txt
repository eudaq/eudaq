 MESSAGE(STATUS "tlu")
  MESSAGE(STATUS "ZESTSC1_INCLUDE_DIR = ${ZESTSC1_INCLUDE_DIR}")
FIND_PACKAGE(ZestSC1 REQUIRED)
#find_package(libusb-1.0 REQUIRED)
find_package(LIBUSB REQUIRED)
find_package(TLUFirmware REQUIRED)

add_definitions(${TLUFIRMWARE_DEFINITIONS})

INCLUDE_DIRECTORIES( include ${ZESTSC1_INCLUDE_DIRS} ${LIBUSB_INCLUDE_DIRS})

add_executable(TLUControl.exe src/TLUControl.cxx src/TLUController.cc src/TLU_USB.cc src/USBTracer.cc src/TLUAddresses1.cc src/TLUAddresses2.cc src/win_uSleep.cc)
add_executable(TLUProducer.exe src/TLUProducer.cxx src/TLUController.cc src/TLU_USB.cc src/USBTracer.cc src/TLUAddresses1.cc src/TLUAddresses2.cc src/win_uSleep.cc)
add_executable(TLUReset.exe src/TLUReset.cxx src/TLUController.cc src/TLU_USB.cc src/USBTracer.cc src/TLUAddresses1.cc src/TLUAddresses2.cc src/win_uSleep.cc)

target_link_libraries(TLUControl.exe   EUDAQ ${EUDAQ_THREADS_LIB} ${LIBUSB_LIBRARIES} ${ZESTSC1_LIBRARIES})
target_link_libraries(TLUProducer.exe   EUDAQ ${EUDAQ_THREADS_LIB} ${LIBUSB_LIBRARIES} ${ZESTSC1_LIBRARIES})
target_link_libraries(TLUReset.exe   EUDAQ ${EUDAQ_THREADS_LIB} ${LIBUSB_LIBRARIES} ${ZESTSC1_LIBRARIES})

INSTALL(TARGETS TLUControl.exe TLUProducer.exe TLUReset.exe
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
