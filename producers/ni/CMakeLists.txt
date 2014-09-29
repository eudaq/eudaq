set(name "NiProducer.exe")
set(sourcefiles src/NiProducer.cxx src/NiController.cc)

# Add Converter plugin to the list of plugins to be baked into the main EUDAQ library:
add_plugin_sources(src/NIConverterPlugin.cc)

INCLUDE_DIRECTORIES( include )
add_executable(${name} ${sourcefiles})

target_link_libraries(${name}   EUDAQ ${EUDAQ_THREADS_LIB})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
