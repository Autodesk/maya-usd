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
if(BUILD_MAYAUSD_LIBRARY)   # requires tinyply.h from include/CMakeLists.txt
    add_executable(testGaussianSplatPly)

    target_sources(testGaussianSplatPly
        PRIVATE
            main.cpp
            test_gaussianSplatPly.cpp
            ${CMAKE_SOURCE_DIR}/lib/mayaUsd/fileio/utils/gaussianSplatPly.cpp
    )

    mayaUsd_compile_config(testGaussianSplatPly)

    target_include_directories(testGaussianSplatPly
        PRIVATE
            ${CMAKE_SOURCE_DIR}/lib
            ${CMAKE_BINARY_DIR}/include
    )

    target_compile_definitions(testGaussianSplatPly
        PRIVATE
            GSPLY_TEST_DATA="${CMAKE_SOURCE_DIR}/test/testSamples/3dgs/ply"
            GSPLY_TEST_OUTPUT="${CMAKE_BINARY_DIR}/test/Temporary/testGaussianSplatPly"
    )

    target_link_libraries(testGaussianSplatPly PRIVATE GTest::GTest)

    mayaUsd_add_test(testGaussianSplatPly
        COMMAND $<TARGET_FILE:testGaussianSplatPly>
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )

    set_property(TEST testGaussianSplatPly APPEND PROPERTY LABELS fileio_utils)
endif()
