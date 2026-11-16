set(TEST_SCRIPT_FILES
    testLdxCapabilityHandler.py
    testLdxConnection.py
    testLdxDebugHandler.py
    testLdxFileHandler.py
)

if (LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION)
    list(APPEND TEST_SCRIPT_FILES testLdxMaterialXUpgrade.py)
endif()

if (LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER)
    list(APPEND TEST_SCRIPT_FILES testLdxComponentConnections.py)
endif()

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_MODULE ${target}
        ENV
            "LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER=${LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER}"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS LookdevXUfe)
endforeach()
