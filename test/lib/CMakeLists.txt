set(TARGET_NAME MAYAUSD_TEST)

# Unit test scripts.
set(test_script_files
    testMayaUsdConverter.py
    testMayaUsdPythonImport.py
)

if (MAYA_APP_VERSION VERSION_GREATER 2020)
    list(APPEND test_script_files
        testMayaUsdProxyAccessor.py
    )
endif()

# copy tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
add_custom_target(${TARGET_NAME} ALL)

mayaUsd_copyFiles(${TARGET_NAME}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${test_script_files})

foreach(script ${test_script_files})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_MODULE ${target}
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )
endforeach()

if (UFE_FOUND)
    add_subdirectory(ufe)
endif()

add_subdirectory(usd)
