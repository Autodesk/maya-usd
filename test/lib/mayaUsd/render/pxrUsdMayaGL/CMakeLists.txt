set(TARGET_NAME MAYAUSD_RENDER_PXRUSDMAYAGL_TEST)

# Unit test scripts.
set(TEST_SCRIPT_FILES
    testProxyShapeDrawAndTransform.py
    testProxyShapeDrawColorAccuracy.py
    testProxyShapeDrawColors.py
    testProxyShapeDrawLighting.py
    testProxyShapeDrawPerformance.py
    testProxyShapeDrawPurpose.py
    testProxyShapeDrawTimeSampled.py
    testProxyShapeDrawUsdChangeProcessing.py
    testProxyShapeDrawVisibility.py
    testProxyShapeDuplicatePerformance.py
    testProxyShapeLiveSurface.py
    testProxyShapeRendererSceneMessages.py
    testProxyShapeSelectionPerformance.py
    testPxrUsdMayaGL.py
)

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
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"

            # Maya uses a very old version of GLEW, so we need support for
            # pre-loading a newer version from elsewhere.
            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"

            # All of the pxrUsdMayaGL tests exercise Pixar's batch renderer, so
            # we turn the Viewport 2.0 render delegate off.
            "MAYAUSD_DISABLE_VP2_RENDER_DELEGATE=1"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS pxrUsdMayaGL)
endforeach()
