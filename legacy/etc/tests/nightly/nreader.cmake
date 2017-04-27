# this script will configure and build the nreader library only
#

find_program(CTEST_GIT_COMMAND NAMES git)

# set variables that describe the tests in the CDash interface
set(CTEST_BUILD_NAME "Nightly-nreader")

# clear binary dir before starting (build directory)
SET (CTEST_START_WITH_EMPTY_BINARY_DIRECTORY TRUE)

set(CTEST_BUILD_CONFIGURATION "Release")
set(CTEST_BUILD_OPTIONS "-DBUILD_main=OFF -DBUILD_python=OFF -DBUILD_gui=OFF -DBUILD_nreader=ON")

# construct the CMake command
set(CTEST_CONFIGURE_COMMAND "${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE:STRING=${CTEST_BUILD_CONFIGURATION}")
set(CTEST_CONFIGURE_COMMAND "${CTEST_CONFIGURE_COMMAND} ${CTEST_BUILD_OPTIONS}")
set(CTEST_CONFIGURE_COMMAND "${CTEST_CONFIGURE_COMMAND} \"-G${CTEST_CMAKE_GENERATOR}\"")
set(CTEST_CONFIGURE_COMMAND "${CTEST_CONFIGURE_COMMAND} \"${CTEST_SOURCE_DIRECTORY}\"")

ctest_start("Nightly")
ctest_update()
ctest_configure()
ctest_build(TARGET install)
ctest_submit()
