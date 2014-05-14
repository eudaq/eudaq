set(Resender_name resender)


INCLUDE_DIRECTORIES( inc )


add_executable(${Resender_name} src/multiResender.cc src/resender.cc src/ResenderProducer.cxx )


target_link_libraries(${Resender_name}   EUDAQ ${EUDAQ_THREADS_LIB} )


INSTALL(TARGETS ${Resender_name} 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
