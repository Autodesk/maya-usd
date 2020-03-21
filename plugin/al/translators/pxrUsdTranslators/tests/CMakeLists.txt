set(TEST_NAME TestPxrUsdTranslators)

mayaUsd_add_test(${TEST_NAME}
    PYTHON_MODULE testPxrTranslators
    ENV
        "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}"
        "TEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
)
