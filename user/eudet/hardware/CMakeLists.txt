include_directories(include)

# library
if(USER_BUILD_TLU)
  include_directories(${ZESTSC1_INCLUDE_DIRS})
  include_directories(${LIBUSB_INCLUDE_DIRS})
  add_definitions(${TLUFIRMWARE_DEFINITIONS})
  list(APPEND USER_HARDWARE_SRC
    src/TLUController.cc
    src/TLUAddresses1.cc
    src/TLUAddresses2.cc)
  list(APPEND USER_HARDWARE_DEP_LIB ${LIBUSB_LIBRARIES} ${ZESTSC1_LIBRARIES})
endif()

if(USER_BUILD_MINITLU)
  include_directories(${CACTUS_INCLUDE_DIR})
  list(APPEND USER_HARDWARE_SRC src/MinitluController.cc)
  list(APPEND USER_HARDWARE_DEP_LIB ${CACTUS_LIBRARIES})
endif()

if(USER_BUILD_NI)
  list(APPEND USER_HARDWARE_SRC src/NiController.cc)
endif()

if(USER_BUILD_TLU OR USER_BUILD_MINITLU OR USER_BUILD_NI)
  add_library(${USER_HARDWARE_LIBRARY} STATIC ${USER_HARDWARE_SRC})
  target_link_libraries(${USER_HARDWARE_LIBRARY} ${EUDAQ_CORE_LIBRARY} ${USER_HARDWARE_DEP_LIB})
  list(APPEND INSTALL_TARGETS ${USER_HARDWARE_LIBRARY})
endif()
# end library

# executable
if(USER_BUILD_TLU)
  add_executable(TLUControl src/TLUControl.cxx)
  add_executable(TLUReset src/TLUReset.cxx)
  target_link_libraries(TLUControl ${EUDAQ_CORE_LIBRARY} ${USER_HARDWARE_LIBRARY})
  target_link_libraries(TLUReset ${EUDAQ_CORE_LIBRARY} ${USER_HARDWARE_LIBRARY})
  list(APPEND INSTALL_TARGETS TLUControl TLUReset)
endif()

if(USER_BUILD_MINITLU)
  add_executable(MinitluControl src/MinitluControl.cxx)
  target_link_libraries(MinitluControl ${EUDAQ_CORE_LIBRARY} ${USER_HARDWARE_LIBRARY})
  set_target_properties(MinitluControl PROPERTIES INSTALL_RPATH
    ${CMAKE_INTSTALL_PREFIX}/lib:${CACTUS_LIBRARY_DIR})
  list(APPEND INSTALL_TARGETS MinitluControl)
endif()
# end executable

install(TARGETS ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

if(WIN32 AND USER_BUILD_TLU AND LIBUSB_WIN_DLL)
  install(FILES
    ${LIBUSB_WIN_DLL}
    DESTINATION bin)
endif()
