
pxr_test_scripts(
    testenv/testPxrUsdTranslators.py
    testenv/testPxrUsdTranslatorsInstancer.py
    testenv/testPxrUsdTranslatorsScope.py
    testenv/testPxrUsdTranslatorsStroke.py
)

set(TEST_INSTALL_PREFIX "${PXR_INSTALL_PREFIX}")

pxr_register_test(testPxrUsdTranslators
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testPxrUsdTranslators"
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/InstancerExportTest
    DEST testPxrUsdTranslatorsInstancer
)
pxr_register_test(testPxrUsdTranslatorsInstancer
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testPxrUsdTranslatorsInstancer"
    TESTENV testPxrUsdTranslatorsInstancer
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/ScopeTest
    DEST testPxrUsdTranslatorsScope
)
pxr_register_test(testPxrUsdTranslatorsScope
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testPxrUsdTranslatorsScope"
    TESTENV testPxrUsdTranslatorsScope
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/StrokeExportTest
    DEST testPxrUsdTranslatorsStroke
)
pxr_register_test(testPxrUsdTranslatorsStroke
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testPxrUsdTranslatorsStroke"
    TESTENV testPxrUsdTranslatorsStroke
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)
