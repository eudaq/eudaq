add_executable(fileReciever    src/fileReciever.cxx)
add_executable(fileSender      src/fileSender.cxx)

set_property(TARGET fileReciever   PROPERTY PROJECT_LABEL "test_fileReciever") 
set_property(TARGET fileSender     PROPERTY PROJECT_LABEL "test_fileSender") 




target_link_libraries(fileReciever    ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})
target_link_libraries(fileSender      ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})


INSTALL(TARGETS fileReciever fileSender
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
