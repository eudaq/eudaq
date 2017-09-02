set(name "DRSProducer.exe")

#FIND_PACKAGE(DRS REQUIRED)

#IF(DRS_FOUND)
#MESSAGE(STATUS "FOUND DRS PATHES")
#ELSE(DRS_FOUND)
#  MESSAGE(FATAL_ERROR "Could not find DRS! Make sure to set the environment variable $DRS_PATH to the path of your pxar installation!")
#ENDIF(DRS_FOUND)

SET(sourcefiles src/DRS4Producer.cxx 
DRS/src/musbstd.c
DRS/src/mxml.c
DRS/src/DRS.cpp
DRS/src/averager.cpp
DRS/src/strlcpy.c)
MESSAGE(STATUS "SOURCEFILES: '" ${sourcefiles} "'")

INCLUDE_DIRECTORIES(include DRS/include)
ADD_EXECUTABLE(${name} ${sourcefiles})

#MESSAGE(STATUS "LIBRARY: " ${DRS_LIBRARY})

MESSAGE(STATUS "ADD_EXECUTABLE " ${name} " " ${sourcefiles})

#TARGET_LINK_LIBRARIES(${name} EUDAQ ${EUDAQ_THREADS_LIB} ${DRS_LIBRARY})
TARGET_LINK_LIBRARIES(${name} EUDAQ ${EUDAQ_THREADS_LIB})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
