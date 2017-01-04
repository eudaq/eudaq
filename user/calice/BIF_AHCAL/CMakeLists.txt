find_package(CACTUS)
cmake_dependent_option(USER_CALICE_BIF "build user/calice BIF" ON
  "USER_CALICE;CACTUS_FOUND" OFF)

if(NOT USER_CALICE_BIF)
  message("user/calice BIF is not to be built")
  return()
endif()

find_package(CACTUS REQUIRED)

include_directories(include ${UHAL_LOG_INCLUDE_PREFIX} ${UHAL_GRAMMARS_INCLUDE_PREFIX} ${EXTERN_BOOST_INCLUDE_PREFIX} ${EXTERN_PUGIXML_INCLUDE_PREFIX} ${UHAL_UHAL_INCLUDE_PREFIX} )

link_directories( ${LINK_DIRECTORIES} ${EXTERN_BOOST_LIB_PREFIX} ${EXTERN_PUGIXML_LIB_PREFIX} ${UHAL_LOG_LIB_PREFIX} ${UHAL_GRAMMARS_LIB_PREFIX} ${UHAL_UHAL_LIB_PREFIX})

add_executable(BIFAHCALProducer  src/caliceahcalbif_Producer.cxx src/caliceahcalbifController.cc )
target_link_libraries(BIFAHCALProducer ${EUDAQ_CORE_LIBRARY} boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal)
list(APPEND INSTALL_TARGETS BIFAHCALProducer)

add_executable(BIFAHCALControl src/caliceahcalbifControl.cxx src/caliceahcalbifController.cc )
target_link_libraries(BIFAHCALControl  ${EUDAQ_CORE_LIBRARY} boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal)
list(APPEND INSTALL_TARGETS BIFAHCALControl)


install(TARGETS ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
