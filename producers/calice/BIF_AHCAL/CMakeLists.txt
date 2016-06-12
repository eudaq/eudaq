
set(name "BIFAHCALProducer")
IF(NOT IGNORE_KNOWN_ISSUES)
  IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
    MESSAGE(WARNING "There are known issues compiling ${name} (see https://github.com/eudaq/eudaq/issues/75). Set the IGNORE_KNOWN_ISSUES option to configure this producer to be build anyway.")
  ENDIF(NOT BUILD_allproducer)
ELSE(NOT IGNORE_KNOWN_ISSUES)
  find_package(CACTUS REQUIRED)

  #add_definitions(${TLUFIRMWARE_DEFINITIONS})

  INCLUDE_DIRECTORIES( include ${UHAL_LOG_INCLUDE_PREFIX} ${UHAL_GRAMMARS_INCLUDE_PREFIX} ${EXTERN_BOOST_INCLUDE_PREFIX} ${EXTERN_PUGIXML_INCLUDE_PREFIX} ${UHAL_UHAL_INCLUDE_PREFIX} )

  LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${EXTERN_BOOST_LIB_PREFIX} ${EXTERN_PUGIXML_LIB_PREFIX} ${UHAL_LOG_LIB_PREFIX} ${UHAL_GRAMMARS_LIB_PREFIX} ${UHAL_UHAL_LIB_PREFIX} )
  add_executable(${name}  src/caliceahcalbif_Producer.cxx src/caliceahcalbifController.cc )
  add_executable(BIFAHCALControl src/caliceahcalbifControl.cxx src/caliceahcalbifController.cc )

  target_link_libraries(BIFAHCALControl  EUDAQ boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal )

  target_link_libraries(${name}  EUDAQ boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal )

  INSTALL(TARGETS BIFAHCALProducer BIFAHCALControl
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF(NOT IGNORE_KNOWN_ISSUES)
