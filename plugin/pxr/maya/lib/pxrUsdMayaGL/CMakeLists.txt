# Unit test scripts.
set(TEST_SCRIPT_FILES
    # Note: the following test fails when switching to the "Expanded"
    #       assembly mode, so disabled it until Pixar figures
    #       what is going wrong.
    # testenv/testRefAssemblyDrawRepresentations.py
    testenv/testBatchRendererIsolateSelection.py
    testenv/testPxrUsdMayaGLInstancerDraw.py
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

            # Fallback to old color management. We will have to investigate
            # and introduce OCIOv2 compatible version of these tests.
            "MAYA_COLOR_MANAGEMENT_SYNCOLOR=1"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS pxrUsdMayaGL)
endforeach()
