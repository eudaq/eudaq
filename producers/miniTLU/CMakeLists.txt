set(name "miniTLUProducer")
# find_package(CACTUS REQUIRED)

set(UHAL_INCLUDE_PREFIX /opt/cactus/include)

set(UHAL_LIB_PREFIX /opt/cactus/lib)

INCLUDE_DIRECTORIES( include ${UHAL_INCLUDE_PREFIX} )
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${UHAL_LIB_PREFIX} )

add_executable(${name} src/miniTLU_Producer.cxx src/miniTLUController.cc )
add_executable(miniTLUControl src/miniTLUControl.cxx src/miniTLUController.cc )

target_link_libraries(miniTLUControl  EUDAQ boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal )

target_link_libraries(${name}  EUDAQ boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal )

INSTALL(TARGETS ${name} miniTLUControl
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


# TODO::check the pugixml boost