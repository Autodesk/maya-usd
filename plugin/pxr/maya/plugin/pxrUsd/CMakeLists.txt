set(PXR_PACKAGE pxrUsd)

pxr_plugin(${PXR_PACKAGE}
    MAYA_PLUGIN

    LIBRARIES
        pxrUsdMayaGL
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

if(CMAKE_WANT_UFE_BUILD)
    find_package(UFE QUIET)
    if(UFE_FOUND)
        message(STATUS "Building with UFE ${UFE_VERSION} features enabled.")
        include_directories(${UFE_INCLUDE_DIR})
        add_definitions(-DWANT_UFE_BUILD)
    else()
        message(STATUS "UFE not found. UFE features will be disabled.")
    endif()
endif()

if (UFE_FOUND)
    target_link_libraries(${PXR_PACKAGE} ${UFE_LIBRARY})
endif()

pxr_test_scripts(
    testenv/testPxrUsdAlembicChaser.py
)

pxr_install_test_dir(
    SRC testenv/AlembicChaser
    DEST testPxrUsdAlembicChaser
)
set(TEST_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/plugin/pxr")

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
