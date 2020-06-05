set(TARGET_NAME MAYAUSD_NODES_TEST)

# Unit test scripts.
set(test_script_files
    testPxrUsdPreviewSurfaceDraw.py
    testPxrUsdPreviewSurfaceExport.py
)

# copy tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
add_custom_target(${TARGET_NAME} ALL)

mayaUsd_copyFiles(${TARGET_NAME}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${test_script_files})


foreach(script ${test_script_files})
    mayaUsd_get_unittest_target(target ${script})
    set(unittest_output_dir
        ${CMAKE_CURRENT_BINARY_DIR}/test_output/${target})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
        ENV
            "TEST_OUTPUT_DIR=${unittest_output_dir}"
    )
endforeach()
