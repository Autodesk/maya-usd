# Unit test scripts.
set(TEST_SCRIPT_FILES "")

list(APPEND TEST_SCRIPT_FILES
    testVP2RenderDelegateDisplayColors.py
	testVP2RenderDelegateGeomSubset.py
    testVP2RenderDelegatePointInstanceOrientation.py
    testVP2RenderDelegateTextureLoading.py
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
    	testVP2RenderDelegateDuplicateProxy.py
        testVP2RenderDelegateInteractiveWorkflows.py
    )
endif()

if (CMAKE_UFE_V2_FEATURES_AVAILABLE AND PXR_VERSION GREATER_EQUAL 2008)
    # The point instance selection tests requires UsdImaging API version 14 or
    # later, which is only provided by core USD 20.08 or later.
    list(APPEND TEST_SCRIPT_FILES
        testVP2RenderDelegatePointInstanceSelection.py
        testVP2RenderDelegatePointInstancesPickMode.py
        testVP2RenderDelegatePrimPath.py
        testVP2RenderDelegateUSDPreviewSurface.py
        testVP2RenderDelegateConsolidation.py
    )
endif()

if (MAYA_APP_VERSION VERSION_GREATER_EQUAL 2022)
    list(APPEND TEST_SCRIPT_FILES
        testVP2RenderDelegatePerInstanceInheritedData.py
        testVP2RenderDelegateBasisCurves.py
		testVP2RenderDelegatePoints.py
    )

    if (PXR_VERSION GREATER_EQUAL 2105)
        list(APPEND TEST_SCRIPT_FILES
            testVP2RenderDelegateDrawModes.py
        )
    endif()
endif()

if (MAYA_APP_VERSION VERSION_GREATER_EQUAL 2022)
    if (CMAKE_UFE_V2_FEATURES_AVAILABLE)
        list(APPEND TEST_SCRIPT_FILES
            testVP2RenderDelegateUsdCamera.py
        )
    endif()
endif()

if (MAYA_APP_VERSION VERSION_GREATER 2022)
    list(APPEND TEST_SCRIPT_FILES
        testVP2RenderDelegateSelection.py
        testVP2RenderDelegateUsdSkel.py
    )
endif()

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4007)
        list(APPEND TEST_SCRIPT_FILES
			testVP2RenderDelegateCameras.py
        )
    endif()
endif()

if (CMAKE_WANT_MATERIALX_BUILD)
    # We want modern OCIO for the MaterialX test:
    mayaUsd_get_unittest_target(target testVP2RenderDelegateMaterialX.py)
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT testVP2RenderDelegateMaterialX.py
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_LIGHTAPI_VERSION=${MAYA_LIGHTAPI_VERSION}"

            # Maya uses a very old version of GLEW, so we need support for
            # pre-loading a newer version from elsewhere.
            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS vp2RenderDelegate)
endif()

if(MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT)
    list(APPEND TEST_SCRIPT_FILES
        testVP2RenderDelegateIsolateSelect.py
    )
endif()

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT ${script}
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_LIGHTAPI_VERSION=${MAYA_LIGHTAPI_VERSION}"

            # Maya uses a very old version of GLEW, so we need support for
            # pre-loading a newer version from elsewhere.
            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"

            # Fallback to old color management. We will have to investigate
            # and introduce OCIOv2 compatible version of these tests.
            "MAYA_COLOR_MANAGEMENT_SYNCOLOR=1"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS vp2RenderDelegate)
endforeach()
