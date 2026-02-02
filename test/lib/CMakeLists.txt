# Unit test scripts.
set(TEST_SCRIPT_FILES
    testMayaUsdConverter.py
    testMayaUsdCreateStageCommands.py
    testMayaUsdCreateStageInMayaRef.py
    testMayaUsdDirtyScene.py
    testMayaUsdLayerEditorCommands.py
    testMayaUsdProxyAccessor.py
    testMayaUsdCacheId.py
    testMayaUsdInfoCommand.py
    testMayaUsdSchemaCommand.py
)

# Interactive Unit test scripts.
set(INTERACTIVE_TEST_SCRIPT_FILES
    testAttributeEditorTemplate.py
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

# The testMayaUsdPythonImport now checks that we can import both the USD Python bindings
# and the MayaUSD Python bindings in a script node context. For this to be tested, we
# need to point to the updated module file and explicitly ask Maya to block execution of
# insecure scripts.
mayaUsd_get_unittest_target(target testMayaUsdPythonImport.py)
mayaUsd_add_test(${target}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PYTHON_MODULE ${target}
    MODULE_PATH ${CMAKE_INSTALL_PREFIX}
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_SECURE_BATCH_ENABLE=1"
)
set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)

# Install a test to check that the MayaHydra plugin loads (if available).
mayaUsd_get_unittest_target(target testMayaHydraPlugin.py)
mayaUsd_add_test(${target}
    INTERACTIVE
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PYTHON_SCRIPT testMayaHydraPlugin.py
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
        "PXR_OVERRIDE_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/lib/usd"
)
set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)

if (AdskUsdComponentCreator_FOUND)
    # Install a test to check that the USD component creator plugin loads (if available).
    mayaUsd_get_unittest_target(target testLoadComponentCreatorPlugin.py)
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT testLoadComponentCreatorPlugin.py
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_MODULE_PATH=${CMAKE_INSTALL_PREFIX}/../AdskUsdComponentCreator"
            "PXR_OVERRIDE_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/lib/usd"
    )
    set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)
else()
    # If MAYAUSD_FORCE_CC_TEST is set to 1, fail the build when component creator is not found
    if (DEFINED ENV{MAYAUSD_FORCE_CC_TEST} AND "$ENV{MAYAUSD_FORCE_CC_TEST}" STREQUAL "1")
        message(FATAL_ERROR "MAYAUSD_FORCE_CC_TEST is set to 1 but AdskUsdComponentCreator was not found. ")
    endif()
endif()

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
add_test(
    NAME "${target}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${MAYA_EXECUTABLE}
)

set_property(TEST ${target} APPEND PROPERTY DISABLED True)
set_property(TEST ${target} APPEND PROPERTY LABELS Debugging)
set_property(TEST ${target} APPEND PROPERTY ENVIRONMENT "MAYA_MODULE_PATH=${CMAKE_INSTALL_PREFIX}")

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
add_subdirectory(ufe)
add_subdirectory(mayaUsd)
add_subdirectory(usd)
if (BUILD_MAYAUSDAPI_LIBRARY)
    add_subdirectory(mayaUsdAPI)
endif()
if(LOOKDEVXUFE_HAS_PYTHON_BINDINGS)
    add_subdirectory(LookdevXUfe)
endif()