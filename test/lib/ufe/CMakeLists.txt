set(TARGET_NAME UFE_TEST)

# unit test scripts.  Note that testTRSBase.py is not a test case, but rather
# a module providing a base class for other tests.
set(test_script_files
    testDeleteCmd.py
    testMatrices.py
    testMayaPickwalk.py
    testRotatePivot.py
	testSelection.py
    testUfePythonImport.py
)

set(test_support_files
    testTRSBase.py
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND test_script_files
        testGroupCmd.py
        testAttribute.py
        testAttributes.py
        testComboCmd.py
        testDuplicateCmd.py
        testMoveCmd.py
        testObject3d.py
        testRename.py
		testParentCmd.py
        testRotateCmd.py
        testScaleCmd.py
        testTransform3dTranslate.py
    )
    if(UFE_PREVIEW_VERSION_NUM GREATER_EQUAL 2009)
        list(APPEND test_script_files
            testContextOps.py
        )
    endif()
endif()

# copy ufe tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
add_custom_target(${TARGET_NAME} ALL)

# copy all the resources and python scripts to build directory
mayaUsd_copyDirectory(${TARGET_NAME}
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    EXCLUDE "*.txt"
)

foreach(script ${test_script_files})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_MODULE ${target}
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_CURRENT_BINARY_DIR}/ufeTestPlugins"
            "UFE_PREVIEW_VERSION_NUM=${UFE_PREVIEW_VERSION_NUM}"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )

    # Assign a ctest label to these tests for easy filtering.
    set_tests_properties(${target} PROPERTIES LABELS ufe)
endforeach()
