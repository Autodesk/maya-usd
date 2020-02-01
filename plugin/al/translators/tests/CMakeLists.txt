mayaUsd_add_pymodule_test(TestAdditionalTranslators
    AL MODULE testTranslators
)

if(IS_LINUX)
    set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT 
        "LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/plugin/al/lib:${PXR_USD_LOCATION}/lib"
    )
endif()
