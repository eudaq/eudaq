set(libname "vmelib" )
set(ext_inc_paths "../../../extern/Tsi148" )

MESSAGE( STATUS "Building VME library with interface classes " )

ADD_DEFINITIONS("-DVME_TRACE=0")
INCLUDE_DIRECTORIES( include ${ext_inc_paths})

AUX_SOURCE_DIRECTORY( src/lib  library_sources)
ADD_LIBRARY( ${libname} SHARED ${library_sources} )

ADD_EXECUTABLE(VMEReset.exe src/exe/VMEReset.cxx)
ADD_EXECUTABLE(VMETest.exe  src/exe/VMETest.cxx)
ADD_EXECUTABLE(Scaler.exe   src/exe/Scaler.cxx)

TARGET_LINK_LIBRARIES(VMEReset.exe   EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${libname})
TARGET_LINK_LIBRARIES(VMETest.exe    EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${libname})
TARGET_LINK_LIBRARIES(Scaler.exe     EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${libname})

INSTALL(TARGETS ${libname} VMEReset.exe VMETest.exe Scaler.exe
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

# also install the vmelib as it is only build when needed but not copied to the lib dir
install(FILES ${CMAKE_BINARY_DIR}/extern/vmelib/libvmelib.so DESTINATION lib)