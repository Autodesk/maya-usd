set(TEST_SCRIPT_FILES
    testComponentTags.py
    testCopyLayerPrims.py
    testPrimReader.py
    testPrimWriter.py
    testExportChaser.py
    testImportChaser.py
    testImportWithNamespace.py
    testJobContextRegistry.py
    testDisplayLayerSaveRestore.py
    testSaveMutedAnonLayer.py
    testSaveLockedAnonLayer.py

    # Once of the tests in this file requires UsdMaya (from the Pixar plugin). That test
    # will be skipped if not found (probably because BUILD_PXR_PLUGIN is off).
    testSchemaApiAdaptor.py
)

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
        testEditAsMayaBBox.py
        testEditAsMayaDefaultValue.py
        testMergeToUsd.py
        testDuplicateAs.py
        testPrimUpdater.py
        testCacheToUsd.py
        testMayaUsdOptions.py
        testSwitchMayaReference.py
    )
endif()

if(UFE_TRIE_NODE_HAS_CHILDREN_COMPONENTS_ACCESSOR)
    list(APPEND TEST_SCRIPT_FILES
        testHideOrphanedNodes.py
    )
endif()

# requires MaterialX support
if (MAYA_APP_VERSION VERSION_GREATER 2022)
    list(APPEND TEST_SCRIPT_FILES
        testShaderReader.py
        testShaderWriter.py
    )
endif()

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ENV
            "UFE_PREVIEW_VERSION_NUM=${UFE_PREVIEW_VERSION_NUM}"
            "USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME=1"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS fileio)
endforeach()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    #------------------------------------------------------------------------------
    # Custom Rig Schema
    #------------------------------------------------------------------------------
    execute_process(COMMAND
                        ${Python_EXECUTABLE}
                        ${USD_GENSCHEMA}
                        ${CMAKE_CURRENT_SOURCE_DIR}/UsdCustomRigSchema/schema.usda
                        ${CMAKE_CURRENT_SOURCE_DIR}/UsdCustomRigSchema/
                    WORKING_DIRECTORY
                        ${CMAKE_CURRENT_BINARY_DIR}
                    RESULT_VARIABLE
                        usdgen_res
    )
    if(usdgen_res)
        message(FATAL_ERROR "Schemas generation failed")
    endif()

    set(TEST_CUSTOM_RIG testCustomRig.py)
    mayaUsd_get_unittest_target(target ${TEST_CUSTOM_RIG})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ENV
            "${PXR_OVERRIDE_PLUGINPATH_NAME}=${CMAKE_CURRENT_SOURCE_DIR}/UsdCustomRigSchema/"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )
endif()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    set(TEST_DISCARD_EDITS testDiscardEdits.py)
    mayaUsd_get_unittest_target(target ${TEST_DISCARD_EDITS})
    if(UFE_TRIE_NODE_HAS_CHILDREN_COMPONENTS_ACCESSOR)
        mayaUsd_add_test(${target}
            PYTHON_MODULE ${target}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "HAS_ORPHANED_NODES_MANAGER=1"
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        )
    else()
        mayaUsd_add_test(${target}
            PYTHON_MODULE ${target}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        )
    endif()

    # Add a ctest label for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS fileio)
endif()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    mayaUsd_get_unittest_target(target testAddMayaReference.py)

    if(UFE_TRIE_NODE_HAS_CHILDREN_COMPONENTS_ACCESSOR)
        mayaUsd_add_test(${target}
            PYTHON_MODULE ${target}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "HAS_ORPHANED_NODES_MANAGER=1"
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        )
    else()
        mayaUsd_add_test(${target}
            PYTHON_MODULE ${target}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        )
    endif()

    # Add a ctest label for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS fileio)
endif()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    mayaUsd_get_unittest_target(target testEditAsMaya.py)

    if(UFE_TRIE_NODE_HAS_CHILDREN_COMPONENTS_ACCESSOR)
        mayaUsd_add_test(${target}
            PYTHON_MODULE ${target}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "HAS_ORPHANED_NODES_MANAGER=1"
                "UFE_PREVIEW_VERSION_NUM=${UFE_PREVIEW_VERSION_NUM}"
                "USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME=1"
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        )
    else()
        mayaUsd_add_test(${target}
            PYTHON_MODULE ${target}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
                "UFE_PREVIEW_VERSION_NUM=${UFE_PREVIEW_VERSION_NUM}"
                "USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME=1"
        )
    endif()

    # Add a ctest label for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS fileio)
endif()

add_subdirectory(utils)
