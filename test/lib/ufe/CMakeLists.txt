# unit test scripts.  Note that testTRSBase.py is not a test case, but rather
# a module providing a base class for other tests.
set(TEST_SCRIPT_FILES
    testAttribute.py
    testAttributeBlock.py
    testAttributes.py
    testBlockedLayerEdit.py
    testChildFilter.py
    testComboCmd.py
    testContextOps.py
    testDefaultPrimCmds.py
    testDuplicateCmd.py
    testDuplicateProxyShape.py
    testEditRouting.py
    testPrimMetadataEditRouting.py
    testGroupCmd.py
    testMatrices.py
    testMayaPickwalk.py
    testMoveCmd.py
    testObject3d.py
    testObservableScene.py
    testParentCmd.py
    testPayloadCommands.py
    testPointInstances.py
    testPythonWrappers.py
    testReferenceCommands.py
    testRelationshipBlock.py
    testRename.py
    testReorderCmd.py
    testRotateCmd.py
    testRotateCmdUndoRedo.py
    testRotatePivot.py
    testPivotConversion.py
    testScaleCmd.py
    testSceneItem.py
    testSelection.py
    testSelectionNotification.py
    testSelectionByArray.py
    testSchemas.py
    testToggleCommands.py
    testTransform3dChainOfResponsibility.py
    testTransform3dTranslate.py
    testTransform3dTranslateWithTimeSamples.py
    testUIInfoHandler.py
    testUfePythonImport.py
    testVariant.py
    testVisibilityCmd.py
)

set(TEST_SUPPORT_FILES
    testTRSBase.py
)

set(INTERACTIVE_TEST_SCRIPT_FILES
    testPixelMoveCmd.py
    testUIIcons.py
)

if(MAYA_APP_VERSION VERSION_GREATER_EQUAL 2023)
    list(APPEND TEST_SCRIPT_FILES
        testCenterPivot.py
    )
endif()


if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
        testUngroupCmd.py
    )
endif()

if (UFE_SCENEITEM_HAS_METADATA)
    list(APPEND TEST_SCRIPT_FILES
        testSceneItemMetadata.py
    )
endif()

if (UFE_SCENE_SEGMENT_SUPPORT)
    list(APPEND TEST_SCRIPT_FILES
        testSceneSegment.py
    )
endif()

if (v4_BatchOps IN_LIST UFE_PREVIEW_FEATURES)
    list(APPEND TEST_SCRIPT_FILES
        testBatchOpsHandler.py
    )
endif()

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
        testAttributeInfo.py
        testConnections.py
        testUINodeGraphNode.py
    )

    if(CMAKE_WANT_MATERIALX_BUILD)
        list(APPEND TEST_SCRIPT_FILES
            testShaderNodeDef.py
        )
    endif()
endif()

if (UFE_LIGHTS_SUPPORT)
    list(APPEND TEST_SCRIPT_FILES
        testLight.py
    )
endif()

if (UFE_MATERIALS_SUPPORT)
    list(APPEND TEST_SCRIPT_FILES
        testMaterial.py
    )
endif()

if (UFE_CLIPBOARD_SUPPORT)
    list(APPEND TEST_SCRIPT_FILES
        testClipboard.py
    )
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

if (MAYA_HAS_DISPLAY_LAYER_API)
    list(APPEND TEST_SCRIPT_FILES
        testDisplayLayer.py
        )
endif()

if(CodeWrapperHandler IN_LIST UFE_PREVIEW_FEATURES)
    set(UFE_HAS_CODE_WRAPPER "1")
else()
    set(UFE_HAS_CODE_WRAPPER "0")
endif()

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_MODULE ${target}
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_CURRENT_SOURCE_DIR}/ufeTestPlugins"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "UFE_HAS_CODE_WRAPPER=${UFE_HAS_CODE_WRAPPER}"
            "USD_HAS_COLOR4_SDR_SUPPORT=${USD_HAS_COLOR4_SDR_SUPPORT}"
            "USD_HAS_MX_METADATA_SUPPORT=${USD_HAS_MX_METADATA_SUPPORT}"
            "UFE_ATTRIBUTES_GET_ENUMS=${UFE_ATTRIBUTES_GET_ENUMS}"
            "UFE_HAS_NATIVE_TYPE_METADATA=${UFE_HAS_NATIVE_TYPE_METADATA}"
            "UFE_UINODEGRAPHNODE_HAS_SIZE=${UFE_UINODEGRAPHNODE_HAS_SIZE}"
            "UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR=${UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR}"
            "UFE_MATERIAL_HAS_HASMATERIAL=${UFE_MATERIAL_HAS_HASMATERIAL}"
            "UFE_CAMERA_HAS_RENDERABLE=${UFE_CAMERA_HAS_RENDERABLE}"
            "UFE_SCENE_SEGMENT_HANDLER_ROOT_PATH=${UFE_SCENE_SEGMENT_HANDLER_ROOT_PATH}"
            "UFE_LIGHTS2_SUPPORT=${UFE_LIGHTS2_SUPPORT}"
            "UFE_VOLUME_LIGHTS_SUPPORT=${UFE_VOLUME_LIGHTS_SUPPORT}"
            "UFE_SCENEITEM_HAS_METADATA=${UFE_SCENEITEM_HAS_METADATA}"
            "HAS_LOOKDEVXUSD=${BUILD_LOOKDEVXUSD_LIBRARY}"
            "UFE_HAS_UNSIGNED_INT=${UFE_HAS_UNSIGNED_INT}"
            "UFE_BATCH_OPS_HAS_DUPLICATE_TO_TARGET=${UFE_BATCH_OPS_HAS_DUPLICATE_TO_TARGET}"
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
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS ufe)
endforeach()
