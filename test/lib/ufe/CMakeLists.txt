set(TARGET_NAME UFE_TEST)

# unit test scripts.  Note that testTRSBase.py is not a test case, but rather
# a module providing a base class for other tests.
set(test_script_files
    testDeleteCmd.py
    testMatrices.py
    testMayaPickwalk.py
    testRotatePivot.py
)

set(test_support_files
    testTRSBase.py
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND test_script_files
        testGroupCmd.py
        testAttribute.py
        testAttributes.py
		# The following files test UFE_V1 interfaces, and therefore should not
		# depend on UFE_V2.  However, the test code relies on capability to
		# retrieve a USD prim from a UFE scene item, which in turn depends on
		# functionality that is only available in UFE_V2.  Therefore, run the
		# tests only if UFE_V2 is available.  PPT, 11-Oct-2019.
		#
		# MAYA-101385: until Maya switches back to UFE_V2, fail, so commented
		# out.  PPT, 11-Oct-2019.
		#
        # testDuplicateCmd.py
        # testMoveCmd.py
        testObject3d.py
        # testRotateCmd.py
        # testScaleCmd.py
        # testTransform3dTranslate.py
        testRename.py
    )
    if(UFE_PREVIEW_VERSION_NUM GREATER_EQUAL 2009)
        list(APPEND test_script_files
            testContextOps.py
        )
    endif()
endif()

# copy ufe tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
add_custom_target(${TARGET_NAME} ALL)

mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY test-samples)
mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY ufeScripts)
mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY ufeTestUtils)
mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY ufeTestPlugins)
mayaUsd_copyFiles(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                         FILES ${test_script_files})
mayaUsd_copyFiles(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                         FILES ${test_support_files})

foreach(script ${test_script_files})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_pymodule_test(${target}
        MAYA_PLUG_IN_PATH
            "${CMAKE_CURRENT_BINARY_DIR}/ufeTestPlugins"
    )
endforeach()
