set(TEST_SCRIPT_FILES
    testReadWriteUtils.py
    testXformStack.py
)

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS fileio_utils)
endforeach()

# -----------------------------------------------------------------------------
# C++ unit tests
# -----------------------------------------------------------------------------
if(BUILD_MAYAUSD_LIBRARY)
    add_mayaUsdLibUtils_test(
        testGaussianSplatPly
        test_gaussianSplatPly.cpp
    )

    target_compile_definitions(testGaussianSplatPly
        PRIVATE
            GSPLY_TEST_DATA="${CMAKE_SOURCE_DIR}/test/testSamples/3dgs/ply"
            GSPLY_TEST_OUTPUT="${CMAKE_BINARY_DIR}/test/Temporary/testGaussianSplatPly"
    )

    set_property(TEST testGaussianSplatPly APPEND PROPERTY LABELS fileio_utils)
endif()
