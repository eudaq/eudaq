add_executable(tasks_simple    src/tasks.cxx)


set_property(TARGET tasks_simple   PROPERTY PROJECT_LABEL "test_tasks_simple") 





target_link_libraries(tasks_simple ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})



INSTALL(TARGETS tasks_simple 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
