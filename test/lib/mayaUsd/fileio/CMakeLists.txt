set(TEST_SCRIPT_FILES
    testPrimReader.py
    testPrimWriter.py
    testExportChaser.py
    testImportChaser.py
    testJobContextRegistry.py
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
        ENV
            "UFE_PREVIEW_VERSION_NUM=${UFE_PREVIEW_VERSION_NUM}"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS fileio)
endforeach()

add_subdirectory(utils)
