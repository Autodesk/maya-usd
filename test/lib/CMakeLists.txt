set(TARGET_NAME MAYAUSD_TEST)

# Unit test scripts.
set(TEST_SCRIPT_FILES
    testMayaUsdConverter.py
    testMayaUsdPythonImport.py
    testMayaUsdLayerEditorCommands.py
    testMayaUsdCacheId.py
)

if (UFE_FOUND AND MAYA_APP_VERSION VERSION_GREATER 2020)
    list(APPEND TEST_SCRIPT_FILES
        testMayaUsdProxyAccessor.py
    )
endif()

# copy tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
add_custom_target(${TARGET_NAME} ALL)

mayaUsd_copyFiles(${TARGET_NAME}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${TEST_SCRIPT_FILES}
)

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_MODULE ${target}
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )
endforeach()

add_subdirectory(mayaUsd)

if (UFE_FOUND)
    add_subdirectory(ufe)
endif()

add_subdirectory(usd)
