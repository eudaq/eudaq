#
# This file defines a number of data-driven tests on the core EUDAQ library using the
# Python ctypes wrapper. If you have access the reference files you
# can run the tests by running 'make test' in the CMake build directory
#

# =============================================================================
# =============================================================================
# Test 1: Dummy data production through Python scripts w/ output verification
# =============================================================================
# =============================================================================

  FIND_PACKAGE(PythonInterp)


  SET( testdir "${PROJECT_BINARY_DIR}/Testing/tests" )
  SET( datadir "${testdir}/data" )
  SET( testscriptdir "${PROJECT_SOURCE_DIR}/etc/tests" )
  SET( pydir "${PROJECT_SOURCE_DIR}/python")

  IF (NOT ReferenceDataDir)
    SET( ReferenceDataDir "/afs/desy.de/group/telescopes/EudaqTestData" )
  ENDIF(NOT ReferenceDataDir)

  # all this regular expressions must be matched for the tests to pass.
  # the order of the expressions must be matched in the test execution!
  # additional statements can be defined for each test individually
  SET( pass_regex_1 "Successfullly started run!" )
  SET( pass_regex_1 "Successfullly finished run!" )

  SET( generic_fail_regex "ERROR" "Error" "CRITICAL" "segmentation violation")


# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#  STEP 0: PREPARE TEST DIRECTORY
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	ADD_TEST(NAME TestSetup COMMAND ${CMAKE_COMMAND}
            	      -D SETUPPATH=${testdir}
            	      -D TESTSCRIPTPATH=${testscriptdir}
   		      -P ${testscriptdir}/setup.cmake)

# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#  STEP 1: EXECUTE DUMMY RUN PRODUCING FIXED DATA SET
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    ADD_TEST( NAME DummyDataProductionRun 
              WORKING_DIRECTORY "${testdir}/data"
	      COMMAND ${PYTHON_EXECUTABLE} ${testscriptdir}/run_dummydataproduction.py --eudaq-pypath=${pydir} )
    SET_TESTS_PROPERTIES (DummyDataProductionRun PROPERTIES
        # test will pass if ALL of the following expressions are matched
        PASS_REGULAR_EXPRESSION "${pass_regex_1}.*${pass_regex_2}"
        # test will fail if ANY of the following expressions is matched 
        FAIL_REGULAR_EXPRESSION "${generic_fail_regex}"
    )


    # now check if the expected output files exist and look ok
    ADD_TEST( NAME DummyDataProductionRefComp 
    	      COMMAND ${PYTHON_EXECUTABLE} "${testscriptdir}/compareRawFiles.py" "${datadir}" "${ReferenceDataDir}/dummyrefrun.raw"
	      )
    SET_TESTS_PROPERTIES (DummyDataProductionRefComp PROPERTIES DEPENDS DummyDataProductionRun)



