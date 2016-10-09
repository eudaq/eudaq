set(name_pro "miniTLUProducer.exe")
set(name_con "miniTLUControl.exe")

# find_package(CACTUS REQUIRED)
set(UHAL_INCLUDE_PREFIX /opt/cactus/include)
set(UHAL_LIB_PREFIX /opt/cactus/lib)
set(UHAL_LIB cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal)
include_directories(${UHAL_INCLUDE_PREFIX} )
link_directories(${LINK_DIRECTORIES} ${UHAL_LIB_PREFIX})

# find_package(pugixml REQUIRED)

# find_package(Boost REQUIRED COMPONENTS thread filesystem regex system program_options)
# include_directories(${Boost_INCLUDE_DIRS})
# set(BOOST_LIB ${Boost_LIBRARIES})
set(BOOST_LIB boost_thread boost_filesystem boost_regex boost_system boost_program_options)

include_directories( include)

add_executable(${name_pro} src/miniTLUProducer.cxx src/miniTLUController.cc )
add_executable(${name_con} src/miniTLUControl.cxx src/miniTLUController.cc )

target_link_libraries(${name_con}  EUDAQ ${UHAL_LIB} ${BOOST_LIB} pugixml)
target_link_libraries(${name_pro}  EUDAQ ${UHAL_LIB} ${BOOST_LIB} pugixml)

INSTALL(TARGETS ${name_pro} ${name_con}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

# TODO::check the pugixml boost
