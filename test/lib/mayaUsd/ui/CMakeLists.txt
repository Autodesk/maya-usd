if (AdskUsdRenderSetup_FOUND)
    set(TEST_SCRIPT_FILES
        testRenderSetupUndo.py
    )

    foreach(script ${TEST_SCRIPT_FILES})
        mayaUsd_get_unittest_target(target ${script})
        mayaUsd_add_test(${target}
            PYTHON_MODULE ${target}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        )

        set_property(TEST ${target} APPEND PROPERTY LABELS undo)
        set_property(TEST ${target} APPEND PROPERTY LABELS SharedComponents)
    endforeach()
endif()
