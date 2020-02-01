set(TEST_NAME TestPxrUsdTranslators)

mayaUsd_add_pymodule_test(${TEST_NAME}
    AL PXR MODULE testPxrTranslators
    PYTHONPATH
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT 
    "TEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
)

if(IS_LINUX)
    set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT 
        "LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/plugin/al/lib:${PXR_USD_LOCATION}/lib"
    )
endif()
