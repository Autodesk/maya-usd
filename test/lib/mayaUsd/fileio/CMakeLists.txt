set(TARGET_NAME MAYAUSD_FILEIO_TEST)

set(TEST_SCRIPT_FILES
    testPrimReader.py
    testPrimWriter.py
    testShaderReader.py
    testShaderWriter.py
    testExportChaser.py
    testImportChaser.py
)
if (MAYA_APP_VERSION VERSION_GREATER_EQUAL 2022)
    list(APPEND TEST_SCRIPT_FILES
        testComponentTags.py
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
