# unit test scripts.  Note that testTRSBase.py is not a test case, but rather
# a module providing a base class for other tests.
set(TEST_SCRIPT_FILES
    testMatrices.py
    testMayaPickwalk.py
    testPythonWrappers.py
    testSelection.py
    testSelectionByArray.py
    testUfePythonImport.py
    testAttributeBlock.py
    testBlockedLayerEdit.py
)

set(TEST_SUPPORT_FILES
    testTRSBase.py
)

set(INTERACTIVE_TEST_SCRIPT_FILES "")

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
        testAttribute.py
        testAttributes.py
        testChildFilter.py
        testComboCmd.py
        testContextOps.py
        testDuplicateCmd.py
        testGroupCmd.py
        testMoveCmd.py
        testObject3d.py
        testRename.py
        testParentCmd.py
        testPointInstances.py
        testReorderCmd.py
        testRotateCmd.py
        testRotatePivot.py
        testScaleCmd.py
        testSceneItem.py
        testTransform3dChainOfResponsibility.py
        testTransform3dTranslate.py
        testUIInfoHandler.py
        testObservableScene.py
    )

    list(APPEND INTERACTIVE_TEST_SCRIPT_FILES
        testPixelMoveCmd.py
        testUIIcons.py
    )
endif()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
        testUngroupCmd.py
    )
endif()

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4017)
        list(APPEND TEST_SCRIPT_FILES
            testAttributeInfo.py
            testConnections.py
        )
    endif()

    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4007)
        list(APPEND TEST_SCRIPT_FILES
            testLight.py
        )
    endif()

    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4013)
        list(APPEND TEST_SCRIPT_FILES
            testSceneSegment.py
        )
    endif()

    if(CMAKE_WANT_MATERIALX_BUILD)
        list(APPEND TEST_SCRIPT_FILES
            testShaderNodeDef.py
        )
    endif()
endif()

if (MAYA_API_VERSION VERSION_GREATER_EQUAL 20220100)
    list(APPEND TEST_SCRIPT_FILES
        testCamera.py
    )
endif()

if(MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT)
    list(APPEND TEST_SCRIPT_FILES
        testSetsCmd.py
    )
endif()

if(MAYA_ENABLE_NEW_PRIM_DELETE)
    list(APPEND TEST_SCRIPT_FILES
        testDeleteCmd.py
    )
else()
    list(APPEND TEST_SCRIPT_FILES
        testLegacyDeleteCmd.py
    )
endif()

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_MODULE ${target}
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_CURRENT_SOURCE_DIR}/ufeTestPlugins"
            "UFE_PREVIEW_VERSION_NUM=${UFE_PREVIEW_VERSION_NUM}"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS ufe)
endforeach()

foreach(script ${INTERACTIVE_TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT ${script}
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS ufe)
endforeach()
