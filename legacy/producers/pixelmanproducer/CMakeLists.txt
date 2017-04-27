set(name "PixelmanProducer.exe")
IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
  MESSAGE(WARNING "There is no CMake configuration file for ${name} yet. Please follow the instructions in the HowTo.txt file contained in the producer's subdirectory to configure it manually.")
ENDIF(NOT BUILD_allproducer)

