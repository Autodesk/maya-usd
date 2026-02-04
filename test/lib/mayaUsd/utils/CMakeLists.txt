# -----------------------------------------------------------------------------
# Python unit tests
# -----------------------------------------------------------------------------

set(TEST_SCRIPT_FILES
    testBlockSceneModificationContext.py
    testDiagnosticDelegate.py
    testUtilsEditability.py
)

if(CMAKE_WANT_MATERIALX_BUILD AND CMAKE_UFE_V3_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
        testMaterialCommands.py
    )
endif()

set(INTERACTIVE_TEST_SCRIPT_FILES
    testUtilsSelectability.py
    testUtilsSelectabilityPointInstanceSelection.py
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
    set_property(TEST ${target} APPEND PROPERTY LABELS utils)
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
    set_property(TEST ${target} APPEND PROPERTY LABELS utils)
endforeach()

# -----------------------------------------------------------------------------
# C++ unit tests
# -----------------------------------------------------------------------------
function(add_mayaUsdLibUtils_test TARGET_NAME)
    add_executable(${TARGET_NAME})

    # -----------------------------------------------------------------------------
    # sources
    # -----------------------------------------------------------------------------
    target_sources(${TARGET_NAME}
        PRIVATE
        main.cpp
        ${ARGN}
    )

    # -----------------------------------------------------------------------------
    # compiler configuration
    # -----------------------------------------------------------------------------
    mayaUsd_compile_config(${TARGET_NAME})

    target_compile_definitions(${TARGET_NAME}
        PRIVATE
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        # Needed by Pixar's wrap_python.hpp
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
    )

    # -----------------------------------------------------------------------------
    # link libraries
    # -----------------------------------------------------------------------------
    target_link_libraries(${TARGET_NAME}
        PRIVATE
        GTest::GTest
        ${MAYA_LIBRARIES}
        mayaUsd
        usdUfe
    )

    # -----------------------------------------------------------------------------
    # unit tests
    # -----------------------------------------------------------------------------
    mayaUsd_add_test(${TARGET_NAME}
        COMMAND $<TARGET_FILE:${TARGET_NAME}>
        ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_LOCATION=${MAYA_LOCATION}"
    )
endfunction()

if(IS_WINDOWS)
    # There are link problems on Linux and OSX with C++ test using USD + Maya,
    # so only run the test on Windows. The code is not platform-specific anwyay,
    # testing on Windows is sufficient.
    add_mayaUsdLibUtils_test(
        testLoadRules
        testLoadRules.cpp
    )
    add_mayaUsdLibUtils_test(
        testUtilsFileSystem
        testUtilsFileSystem.cpp
    )
    add_mayaUsdLibUtils_test(
        testTargetLayer
        testTargetLayer.cpp
    )
    add_mayaUsdLibUtils_test(
        testSplitString
        testSplitString.cpp
    )

    if(CMAKE_WANT_MATERIALX_BUILD AND PXR_VERSION GREATER_EQUAL 2211)
        add_mayaUsdLibUtils_test(
            test_ShaderGenUtils
            test_ShaderGenUtils.cpp
        )

        target_compile_definitions(test_ShaderGenUtils
        PRIVATE
            MATERIALX_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/materialx_test_data"
            MATERIALX_TEST_OUTPUT="${CMAKE_BINARY_DIR}/test/Temporary/test_ShaderGenUtils"
        )

        target_link_libraries(test_ShaderGenUtils
        PRIVATE
            usdMtlx
            MaterialXCore
            MaterialXFormat
        )

    endif()    
endif()
