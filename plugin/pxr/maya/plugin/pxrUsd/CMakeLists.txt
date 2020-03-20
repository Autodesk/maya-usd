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

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}

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

if (UFE_FOUND)
    target_link_libraries(${PXR_PACKAGE} ${UFE_LIBRARY})
    target_compile_definitions(${PXR_PACKAGE} PRIVATE WANT_UFE_BUILD)
    target_include_directories(${PXR_PACKAGE} PRIVATE ${UFE_INCLUDE_DIR})
endif()

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
)
