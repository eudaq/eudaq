set(namecontrol "PIControl.exe")
set(namecontroller "PIController.exe")

FIND_PACKAGE(PIdll REQUIRED)

IF(PIWrapper_FOUND)
ELSE()
  MESSAGE(FATAL_ERROR "Couldn't find lib or header!")
ENDIF()

set(sourcefilecontrol src/PIWrapper.cc src/PIControl.cxx)
set(sourcefilecontroller src/PIWrapper.cc src/PIController.cxx)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/include/lib/PI_GCS2_DLL.dll" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../../bin")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(${namecontrol} ${sourcefilecontrol})
add_executable(${namecontroller} ${sourcefilecontroller})

##MESSAGE(${PIWrapper_LIBRARIES})
target_link_libraries(${namecontrol} EUDAQ ${PIWrapper_LIBRARIES})
target_link_libraries(${namecontroller} EUDAQ ${PIWrapper_LIBRARIES})

##MESSAGE("EUDAQ" EUDAQ ${PIWrapper_LIBRARIES} "   " ${PIWrapper_INCLUDE_DIRS})

INSTALL(TARGETS PIControl.exe PIController.exe
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)