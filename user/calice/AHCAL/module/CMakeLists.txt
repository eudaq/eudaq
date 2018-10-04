include_directories(include)
aux_source_directory(src MODULE_SRC)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


# set(sourcefiles src/AHCALProducer.cxx src/AHCALProducer.cc src/ScReader.cc)

# include_directories(./include)
# add_executable(AHCALProducer ${sourcefiles})
# target_link_libraries(AHCALProducer ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})

# install(TARGETS AHCALProducer
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib)
