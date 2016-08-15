# build main EUDAQ executables (with option to disable them, use 'cmake -DMainExec=OFF ..'
option(BUILD_main "Compile main EUDAQ executables?" ON)
IF(BUILD_main)
  add_subdirectory(iphc)
  
  add_executable(ClusterExtractor    src/ClusterExtractor.cxx  )
  add_executable(ExampleReader       src/ExampleReader.cxx     )
  add_executable(FileChecker         src/FileChecker.cxx       )
  add_executable(OptionExample       src/OptionExample.cxx     )
  add_executable(RunListener         src/RunListener.cxx       )
  add_executable(TestLogCollector    src/TestLogCollector.cxx  )
  add_executable(TestProducer        src/TestProducer.cxx      )
  add_executable(TestRunControl      src/TestRunControl.cxx    )

  set_property(TARGET ClusterExtractor   PROPERTY PROJECT_LABEL "bin_ClusterExtractor")  
  set_property(TARGET ExampleReader      PROPERTY PROJECT_LABEL "bin_ExampleReader") 
  set_property(TARGET FileChecker        PROPERTY PROJECT_LABEL "bin_FileChecker") 
  set_property(TARGET OptionExample      PROPERTY PROJECT_LABEL "bin_OptionExample") 
  set_property(TARGET RunListener        PROPERTY PROJECT_LABEL "bin_RunListener") 
  set_property(TARGET TestLogCollector   PROPERTY PROJECT_LABEL "bin_TestLogCollector")
  set_property(TARGET TestProducer       PROPERTY PROJECT_LABEL "bin_TestProducer") 
  set_property(TARGET TestRunControl     PROPERTY PROJECT_LABEL "bin_TestRunControl") 




  # ${ADDITIONAL_LIBRARIES} is only set if e.g. the native reader processor is built (EUTelescope/LCIO)
  target_link_libraries(ClusterExtractor    EUDAQ ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})
  target_link_libraries(ExampleReader       EUDAQ ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})
  target_link_libraries(FileChecker         EUDAQ ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})
  target_link_libraries(OptionExample       EUDAQ ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})
  target_link_libraries(RunListener         EUDAQ ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})
  target_link_libraries(TestLogCollector    EUDAQ ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})
  target_link_libraries(TestProducer        EUDAQ ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})
  target_link_libraries(TestRunControl      EUDAQ ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})


  INSTALL(TARGETS ClusterExtractor  ExampleReader  FileChecker  OptionExample  RunListener  TestLogCollector  TestProducer TestRunControl 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

ENDIF(BUILD_main)
