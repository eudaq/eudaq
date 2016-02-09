FIND_PACKAGE(ZestSC1 REQUIRED)
find_package(LIBUSB REQUIRED)
find_package(TLUFirmware REQUIRED)

add_definitions(${TLUFIRMWARE_DEFINITIONS})

INCLUDE_DIRECTORIES( include ${ZESTSC1_INCLUDE_DIRS} ${LIBUSB_INCLUDE_DIRS})

add_executable(TLUControl src/TLUControl.cxx src/TLUController.cc src/TLU_USB.cc src/USBTracer.cc src/TLUAddresses1.cc src/TLUAddresses2.cc src/win_uSleep.cc)
add_executable(TLUProducer src/TLUProducer.cxx src/TLUController.cc src/TLU_USB.cc src/USBTracer.cc src/TLUAddresses1.cc src/TLUAddresses2.cc src/win_uSleep.cc)
add_executable(TLUReset src/TLUReset.cxx src/TLUController.cc src/TLU_USB.cc src/USBTracer.cc src/TLUAddresses1.cc src/TLUAddresses2.cc src/win_uSleep.cc)

target_link_libraries(TLUControl   EUDAQ ${EUDAQ_THREADS_LIB} ${LIBUSB_LIBRARIES} ${ZESTSC1_LIBRARIES})
target_link_libraries(TLUProducer   EUDAQ ${EUDAQ_THREADS_LIB} ${LIBUSB_LIBRARIES} ${ZESTSC1_LIBRARIES})
target_link_libraries(TLUReset   EUDAQ ${EUDAQ_THREADS_LIB} ${LIBUSB_LIBRARIES} ${ZESTSC1_LIBRARIES})

INSTALL(TARGETS TLUControl TLUProducer TLUReset
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
