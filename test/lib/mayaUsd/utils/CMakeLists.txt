set(TEST_SCRIPT_FILES
    testBlockSceneModificationContext.py
    testDiagnosticDelegate.py
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
        testUtilsEditability.py
    )
endif()

set(INTERACTIVE_TEST_SCRIPT_FILES "")
if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND INTERACTIVE_TEST_SCRIPT_FILES
        testUtilsSelectability.py
        testUtilsSelectabilityPointInstanceSelection.py
    )
endif()
        
foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
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
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS utils)
endforeach()
