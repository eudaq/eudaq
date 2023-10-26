find_package(ROOT REQUIRED COMPONENTS Core Hist)
INCLUDE_DIRECTORIES(${ROOT_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(include)

ADD_EXECUTABLE("desynccorr" src/desynccorr.cxx)
ADD_EXECUTABLE("yarr_resync" src/yarr_resync.cxx)

target_link_libraries("yarr_resync" EUDAQ)
target_link_libraries("desynccorr" EUDAQ ROOT::Core ROOT::Hist)

SET(name "desynccorr" "yarr_resync")
INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin COMPONENT MAIN_EXE
  LIBRARY DESTINATION lib COMPONENT MAIN_EXE
  ARCHIVE DESTINATION lib COMPONENT MAIN_EXE)