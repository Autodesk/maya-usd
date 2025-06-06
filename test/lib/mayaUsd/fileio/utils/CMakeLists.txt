if (USD_SUPPORT_INDIVIDUAL_TRANSFROMS)
    set(TEST_SCRIPT_FILES
        testReadWriteUtils.py
        #need to re-add this test once properly fixed
        #testXformStackSeparateTransforms.py
    )
else()
    set(TEST_SCRIPT_FILES
        testReadWriteUtils.py
        testXformStack.py
    )
endif()

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
