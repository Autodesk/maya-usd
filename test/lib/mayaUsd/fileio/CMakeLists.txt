set(TARGET_NAME MAYAUSD_FILEIO_TEST)

set(TEST_SCRIPT_FILES
    testPrimReader.py
    testPrimWriter.py
    testExportChaser.py
    testImportChaser.py
    testJobContextRegistry.py
    testSchemaApiAdaptor.py
)

# requires MaterialX support
if (MAYA_APP_VERSION VERSION_GREATER 2022 AND PXR_VERSION VERSION_GREATER 2102)
    list(APPEND TEST_SCRIPT_FILES
        testShaderReader.py
        testShaderWriter.py
    )
endif()

add_custom_target(${TARGET_NAME} ALL)

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ENV
            "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}"

    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS fileio)
endforeach()

add_subdirectory(utils)
