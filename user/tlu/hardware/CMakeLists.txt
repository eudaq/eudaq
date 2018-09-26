include_directories(include)

# library
if(USER_TLU_BUILD_EUDET)
  include_directories(${ZESTSC1_INCLUDE_DIRS})
  include_directories(${LIBUSB_INCLUDE_DIRS})
  add_definitions(${TLUFIRMWARE_DEFINITIONS})
  list(APPEND USER_HARDWARE_SRC
    src/EudetTluController.cc
    src/EudetTluAddresses1.cc
    src/EudetTluAddresses2.cc)
  list(APPEND USER_HARDWARE_DEP_LIB ${LIBUSB_LIBRARIES} ${ZESTSC1_LIBRARIES})
endif()

if(USER_TLU_BUILD_AIDA)
  include_directories(${CACTUS_INCLUDE_DIR})
  list(APPEND USER_HARDWARE_SRC 
    src/AidaTluController.cc
    src/AidaTluHardware.cc
    src/AidaTluPowerModule.cc
    src/AidaTluI2c.cc
    src/AidaTluDisplay.cc)
  list(APPEND USER_HARDWARE_DEP_LIB ${CACTUS_LIBRARIES})
endif()

if(USER_TLU_BUILD_EUDET OR USER_TLU_BUILD_AIDA)
  add_library(${USER_HARDWARE_LIBRARY} STATIC ${USER_HARDWARE_SRC})
  target_link_libraries(${USER_HARDWARE_LIBRARY} ${EUDAQ_CORE_LIBRARY} ${USER_HARDWARE_DEP_LIB})
  list(APPEND INSTALL_TARGETS ${USER_HARDWARE_LIBRARY})
endif()
# end library

install(TARGETS ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

if(WIN32 AND USER_BUILD_EUDET_TLU AND LIBUSB_WIN_DLL)
  message("${LIBUSB_WIN_DLL} will copy to bin/")
  install(FILES
    ${LIBUSB_WIN_DLL}
    DESTINATION bin
    RENAME libusb0.dll
    )
endif()
