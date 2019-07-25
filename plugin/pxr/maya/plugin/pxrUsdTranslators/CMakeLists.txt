pxr_test_scripts(
    testenv/testPxrUsdTranslators.py
    testenv/testPxrUsdTranslatorsScope.py
)

pxr_register_test(testPxrUsdTranslators
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPxrUsdTranslators"
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/ScopeTest
    DEST testPxrUsdTranslatorsScope
)
pxr_register_test(testPxrUsdTranslatorsScope
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPxrUsdTranslatorsScope"
    TESTENV testPxrUsdTranslatorsScope
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)
