add_test(
    NAME TestPxrUsdTranslators
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND
        testPxrTranslators.sh
        ${CMAKE_INSTALL_PREFIX}
        ${USD_LIBRARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${USD_MAYA_ROOT}
    )
