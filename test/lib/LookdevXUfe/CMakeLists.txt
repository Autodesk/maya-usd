set(TEST_SCRIPT_FILES
    testLdxCapabilityHandler.py
    testLdxComponentConnections.py        
    testLdxConnection.py
    testLdxDebugHandler.py
    testLdxFileHandler.py
)

if (MaterialX_VERSION VERSION_GREATER "1.39.0")
    list(APPEND TEST_SCRIPT_FILES testLdxMaterialXUpgrade.py)
endif()

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_MODULE ${target}
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS LookdevXUfe)
endforeach()
