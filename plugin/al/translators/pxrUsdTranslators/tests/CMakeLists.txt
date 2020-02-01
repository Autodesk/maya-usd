set(TEST_NAME TestPxrUsdTranslators)

mayaUsd_add_pymodule_test(${TEST_NAME}
    AL PXR MODULE testPxrTranslators
    PYTHONPATH
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT 
    "TEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
)
