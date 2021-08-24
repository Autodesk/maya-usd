set(TARGET_NAME VP2_RENDER_DELEGATE_TEST)

# Unit test scripts.
set(TEST_SCRIPT_FILES "")

list(APPEND TEST_SCRIPT_FILES
	testVP2RenderDelegateGeomSubset.py
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
    	testVP2RenderDelegateDuplicateProxy.py
        testVP2RenderDelegateSelectability.py
        testVP2RenderDelegateSelectabilityPointInstanceSelection.py
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

if (CMAKE_WANT_MATERIALX_BUILD)
    list(APPEND TEST_SCRIPT_FILES
        testVP2RenderDelegateMaterialX.py
    )
endif()

add_custom_target(${TARGET_NAME} ALL)

# Copy all the resources and Python scripts to build directory
mayaUsd_copyDirectory(${TARGET_NAME}
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    EXCLUDE "*.txt"
)

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
            "MAYAUSD_VP2_ENABLE_V2_LIGHTING_SHADER=1"

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
