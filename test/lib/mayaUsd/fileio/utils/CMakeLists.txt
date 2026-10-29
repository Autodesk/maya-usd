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
function(add_mayaUsdFileioUtils_test TARGET_NAME)
    add_executable(${TARGET_NAME})

    # Uses the main.cpp from this directory.
    target_sources(${TARGET_NAME}
        PRIVATE
        main.cpp
        ${ARGN}
    )

    mayaUsd_compile_config(${TARGET_NAME})

    target_compile_definitions(${TARGET_NAME}
        PRIVATE
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        # Needed by Pixar's wrap_python.hpp
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
    )

    target_link_libraries(${TARGET_NAME}
        PRIVATE
        GTest::GTest
        ${MAYA_LIBRARIES}
        mayaUsd
        usdUfe
    )

    mayaUsd_add_test(${TARGET_NAME}
        COMMAND $<TARGET_FILE:${TARGET_NAME}>
        ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_LOCATION=${MAYA_LOCATION}"
    )
endfunction()

if(IS_WINDOWS)
  if(BUILD_MAYAUSD_LIBRARY)
    add_mayaUsdFileioUtils_test(testGaussianSplatPly test_gaussianSplatPly.cpp)

    target_compile_definitions(
      testGaussianSplatPly
      PRIVATE
        GSPLY_TEST_DATA="${CMAKE_SOURCE_DIR}/test/testSamples/3dgs/ply"
        GSPLY_TEST_OUTPUT="${CMAKE_BINARY_DIR}/test/Temporary/testGaussianSplatPly"
    )

    set_property(
      TEST testGaussianSplatPly
      APPEND
      PROPERTY LABELS fileio_utils)
  endif()
endif()
