include_directories(${EUDAQ_INCLUDE_DIRS})

# Define interface library for all EUDAQ modules:

add_subdirectory(example)
add_subdirectory(experimental)
add_subdirectory(eudet)
add_subdirectory(calice)
add_subdirectory(caribou)
add_subdirectory(itkstrip)
add_subdirectory(timepix3)
add_subdirectory(tlu)
add_subdirectory(stcontrol)
add_subdirectory(aidastrip)
add_subdirectory(tbscDESY)
add_subdirectory(MuPix8)
add_subdirectory(piStage)
add_subdirectory(cms-phase2)
