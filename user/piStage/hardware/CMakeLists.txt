include_directories(include)
include_directories(/usr/local/PI/include/)
# library
if(USER_PISTAGE_BUILD)
  list(APPEND USER_HARDWARE_SRC src/RotaController.cc)
endif()

if(USER_PISTAGE_BUILD)
  add_library(${USER_HARDWARE_LIBRARY} STATIC ${USER_HARDWARE_SRC})
  target_link_libraries(${USER_HARDWARE_LIBRARY}
                        ${EUDAQ_CORE_LIBRARY}
                        ${USER_HARDWARE_DEP_LIB}
                        pi_pi_gcs2)
  list(APPEND INSTALL_TARGETS ${USER_HARDWARE_LIBRARY})
endif()
# end library

install(TARGETS ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
