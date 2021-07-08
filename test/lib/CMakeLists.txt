set(TARGET_NAME MAYAUSD_TEST)

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

# copy tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
add_custom_target(${TARGET_NAME} ALL)

mayaUsd_copyFiles(${TARGET_NAME}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${TEST_SCRIPT_FILES} ${INTERACTIVE_TEST_SCRIPT_FILES}
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
            "MAYA_LIGHTAPI_VERSION=${MAYA_LIGHTAPI_VERSION}"

            # Maya uses a very old version of GLEW, so we need support for
            # pre-loading a newer version from elsewhere.
            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"

            # Fallback to old color management. We will have to investigate
            # and introduce OCIOv2 compatible version of these tests.
            "MAYA_COLOR_MANAGEMENT_SYNCOLOR=1"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)
endforeach()

#
# -----------------------------------------------------------------------------
# Special case for plug tests
# -----------------------------------------------------------------------------
set(TEST_PLUG_FILES
    testMayaUsdPlugVersionCheck.py
)

mayaUsd_copyFiles(${TARGET_NAME}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${TEST_PLUG_FILES}
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
