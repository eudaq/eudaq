include_directories(${PROJECT_SOURCE_DIR}/main/lib/processors/include)
include_directories(${PROJECT_SOURCE_DIR}/main/include/eudaq)

option(BUILD_Tests "Compile tests" OFF)
IF(BUILD_Tests)
  add_subdirectory(network)
  add_subdirectory(tasks)
ENDIF()
