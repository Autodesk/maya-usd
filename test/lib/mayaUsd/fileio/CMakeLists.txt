set(TEST_SCRIPT_FILES
    testPrimReader.py
    testPrimWriter.py
    testExportChaser.py
    testImportChaser.py
    testImportWithNamespace.py
    testJobContextRegistry.py
    testDisplayLayerSaveRestore.py

    # Once of the tests in this file requires UsdMaya (from the Pixar plugin). That test
    # will be skipped if not found (probably because BUILD_PXR_PLUGIN is off).
    testSchemaApiAdaptor.py
)
if (MAYA_APP_VERSION VERSION_GREATER_EQUAL 2022)
    list(APPEND TEST_SCRIPT_FILES
        testComponentTags.py
    )
endif()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
        testAddMayaReference.py
        testEditAsMaya.py
        testMergeToUsd.py
        testDiscardEdits.py
        testDuplicateAs.py
        testPrimUpdater.py
        testCacheToUsd.py
        testMayaUsdOptions.py
    )
endif()

# requires MaterialX support
if (MAYA_APP_VERSION VERSION_GREATER 2022 AND PXR_VERSION VERSION_GREATER 2102)
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
    )
endif()

add_subdirectory(utils)
