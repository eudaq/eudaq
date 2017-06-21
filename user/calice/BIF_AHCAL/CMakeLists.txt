cmake_dependent_option(USER_CALICE_BUILD_BIF "build user/calice BIF" ON
  "USER_CALICE_BUILD;CACTUS_FOUND" OFF)

if(NOT USER_CALICE_BUILD_BIF)
  message("user/calice BIF is not to be built")
  return()
endif()

find_package(CACTUS)

if(NOT CACTUS_FOUND)
  message(WARNING "CACTUS/uhal is not found, user/calice BIF can not be built")
  find_package(CACTUS REQUIRED)
endif()

include_directories(include ${CACTUS_INCLUDE_DIR})

add_executable(BIFAHCALProducer  src/caliceahcalbif_Producer.cxx src/caliceahcalbifController.cc )
target_link_libraries(BIFAHCALProducer ${EUDAQ_CORE_LIBRARY} ${CACTUS_LIBRARIES})
set_target_properties(BIFAHCALProducer PROPERTIES INSTALL_RPATH
  ${EUDAQ_INSTALL_RPATH}:${CACTUS_LIBRARY_DIR})
list(APPEND INSTALL_TARGETS BIFAHCALProducer)

add_executable(BIFAHCALControl src/caliceahcalbifControl.cxx src/caliceahcalbifController.cc )
target_link_libraries(BIFAHCALControl ${EUDAQ_CORE_LIBRARY} ${CACTUS_LIBRARIES})
set_target_properties(BIFAHCALControl PROPERTIES INSTALL_RPATH
  ${EUDAQ_INSTALL_RPATH}:${CACTUS_LIBRARY_DIR})
list(APPEND INSTALL_TARGETS BIFAHCALControl)

install(TARGETS ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
