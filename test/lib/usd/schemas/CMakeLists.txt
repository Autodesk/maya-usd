set(TARGET_NAME MAYAUSD_SCHEMA_TEST)

# Unit test scripts.
set(test_script_files
    testMayaUsdSchemasPythonImport.py
    testMayaUsdSchemasMayaReference.py
)

# copy tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
add_custom_target(${TARGET_NAME} ALL)

mayaUsd_copyFiles(${TARGET_NAME}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${test_script_files})


foreach(script ${test_script_files})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
    )
endforeach()
