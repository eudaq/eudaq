include_directories(include)
include_directories(/usr/local/PI/include/)
# library
if(USER_PISTAGE_BUILD)
  list(APPEND USER_HARDWARE_SRC src/RotaController.cc)
endif()

find_library(PI_FOUND NAMES pi_pi_gcs2 HINTS "/usr/local/PI")
if(NOT PI_FOUND)
  message(FATAL_ERROR "PI  library not found")
else()
    message("Found stage controller lib at ${PI_FOUND}")
endif()

if(USER_PISTAGE_BUILD AND PI_FOUND)
  add_library(${USER_HARDWARE_LIBRARY} STATIC ${USER_HARDWARE_SRC})
  target_link_libraries(${USER_HARDWARE_LIBRARY}
                        ${EUDAQ_CORE_LIBRARY}
                        ${USER_HARDWARE_DEP_LIB}
                        ${PI_FOUND})
  list(APPEND INSTALL_TARGETS ${USER_HARDWARE_LIBRARY})
endif()
# end library

install(TARGETS ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
