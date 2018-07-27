# cmake_dependent_option(USER_CMSHGCAL_BUILD "build user/cmshgcal" ON
#   "USER_CMSHGCAL_BUILD" OFF)

# if(NOT USER_CMSHGCAL_BUILD)
#   message("user/cmshgcal is not to be built")
#   return()
# endif()

# set(Boost_USE_STATIC_LIBS OFF) 
# set(Boost_USE_MULTITHREADED ON)  
# set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost) 
find_package(CACTUS REQUIRED)
find_package(ROOT)

include_directories(include ${Boost_INCLUDE_DIRS} ${UHAL_LOG_INCLUDE_PREFIX} ${UHAL_GRAMMARS_INCLUDE_PREFIX} ${EXTERN_BOOST_INCLUDE_PREFIX} ${EXTERN_PUGIXML_INCLUDE_PREFIX} ${UHAL_UHAL_INCLUDE_PREFIX} ${ROOT_INCLUDE_DIR})
link_directories( ${link_directories} ${EXTERN_BOOST_LIB_PREFIX} ${EXTERN_PUGIXML_LIB_PREFIX} ${UHAL_LOG_LIB_PREFIX} ${UHAL_GRAMMARS_LIB_PREFIX} ${UHAL_UHAL_LIB_PREFIX} )
aux_source_directory(src MODULE_SRC)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} boost_timer boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options boost_iostreams cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal ${ROOT_LIBRARIES})

add_executable(HGCalOnly_DAQ ${MODULE_SRC})
target_link_libraries(HGCalOnly_DAQ ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} boost_timer boost_thread boost_filesystem boost_regex boost_system boost_thread boost_program_options boost_iostreams cactus_extern_pugixml cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal ${ROOT_LIBRARIES})


install(TARGETS ${EUDAQ_MODULE} HGCalOnly_DAQ
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
