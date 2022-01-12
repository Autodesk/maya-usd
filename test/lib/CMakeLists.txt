# Unit test scripts.
set(TEST_SCRIPT_FILES
    testMayaUsdConverter.py
    testMayaUsdCreateStageCommands.py
    testMayaUsdPythonImport.py
    testMayaUsdLayerEditorCommands.py
    testMayaUsdCacheId.py
)

if (UFE_FOUND AND MAYA_APP_VERSION VERSION_GREATER 2020)
    list(APPEND TEST_SCRIPT_FILES
        testMayaUsdDirtyScene.py
        testMayaUsdProxyAccessor.py
    )
endif()

# Interactive Unit test scripts.
set(INTERACTIVE_TEST_SCRIPT_FILES
    testMayaUsdInteractiveLayerEditorCommands.py
)

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_MODULE ${target}
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)
endforeach()

foreach(script ${INTERACTIVE_TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT ${script}
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)
endforeach()

#
# -----------------------------------------------------------------------------
# Special interactive test to be able to start Maya with test configurations.
# Can be started with VS Code but won't be part of test suites.
# -----------------------------------------------------------------------------
set(target _Interactive_Maya)
mayaUsd_add_test(${target}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${MAYA_EXECUTABLE}
    ENV
        "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
set_property(TEST ${target} APPEND PROPERTY DISABLED True)
set_property(TEST ${target} APPEND PROPERTY LABELS Debugging)

#
# -----------------------------------------------------------------------------
# Special case for plug tests
# -----------------------------------------------------------------------------
set(TEST_PLUG_FILES
    testMayaUsdPlugVersionCheck.py
)

foreach(script ${TEST_PLUG_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_MODULE ${target}
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_PXR_PLUGINPATH_NAME=${CMAKE_CURRENT_BINARY_DIR}/usd/plugin/TestMayaUsdPlug"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)
endforeach()

# -----------------------------------------------------------------------------
if (UFE_FOUND)
    add_subdirectory(ufe)
    add_subdirectory(mayaUsd)
endif()

add_subdirectory(usd)
