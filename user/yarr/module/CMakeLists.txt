include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_TTREE_LIBRARY)
  #list(REMOVE_ITEM MODULE_SRC src/Ex0RawEvent2TTreeEventConverter.cc)
endif()
if(NOT EUDAQ_ROOT_MONITOR_LIBRARY)
  #list(REMOVE_ITEM MODULE_SRC src/Ex0ROOTMonitor.cc)
endif()

set(JSONCONS_BUILD_TESTS OFF CACHE INTERNAL "")
include(FetchContent)
FetchContent_Declare(extern_jsoncons
GIT_REPOSITORY https://github.com/danielaparker/jsoncons.git
GIT_TAG v0.176.0
GIT_SHALLOW 1
)
FetchContent_MakeAvailable(extern_jsoncons)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} jsoncons)

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
