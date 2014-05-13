find_program(CTEST_GIT_COMMAND NAMES git)

# clear binary dir before starting (build directory)
SET (CTEST_START_WITH_EMPTY_BINARY_DIRECTORY TRUE)

# valgrind is used for memchecks
find_program(CTEST_MEMORYCHECK_COMMAND NAMES valgrind)

# remove python related valgrind messages
set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE ${CTEST_SOURCE_DIRECTORY}/etc/tests/valgrind/valgrind-python.supp)


ctest_start("Nightly")
ctest_update()
ctest_configure()
ctest_build(TARGET install)
ctest_test()
if (WITH_MEMCHECK)
  ctest_memcheck()
endif (WITH_MEMCHECK)
ctest_submit()
