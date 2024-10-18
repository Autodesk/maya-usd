set(PXR_PACKAGE pxrUsd)

pxr_plugin(${PXR_PACKAGE}
    MAYA_PLUGIN

    LIBRARIES
        mayaUsd
        sdf
        tf
        usd
        usdGeom
        usdMaya
        ${MAYA_Foundation_LIBRARY}
        ${MAYA_OpenMaya_LIBRARY}
        ${MAYA_OpenMayaRender_LIBRARY}
        ${UFE_LIBRARY}

    INCLUDE_DIRS
        ${PXR_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}
        ${UFE_INCLUDE_DIR}

    CPPFILES
        alembicChaser.cpp
        plugin.cpp
        usdTranslator.cpp

    PRIVATE_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)

pxr_test_scripts(
    testenv/testPxrUsdAlembicChaser.py
)

pxr_install_test_dir(
    SRC testenv/AlembicChaser
    DEST testPxrUsdAlembicChaser
)
set(TEST_INSTALL_PREFIX "${PXR_INSTALL_PREFIX}")

pxr_register_test(testPxrUsdAlembicChaser
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testPxrUsdAlembicChaser"
    TESTENV testPxrUsdAlembicChaser
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
        WIN_DLL_PATH "${PXR_USD_LOCATION}/plugin/usd;${PXR_USD_LOCATION}/lib;${PXR_USD_LOCATION}/bin;${TEST_INSTALL_PREFIX}/maya/lib;${CMAKE_INSTALL_PREFIX}/lib"
)
