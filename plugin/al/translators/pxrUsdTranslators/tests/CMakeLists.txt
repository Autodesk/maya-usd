set(TEST_NAME TestPxrUsdTranslators)

set(path 
    "${CMAKE_INSTALL_PREFIX}/lib"
    "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib"
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib"
    "$ENV{PATH}"
)

set(pythonPath 
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/python"
    "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib/python"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "$ENV{PYTHONPATH}"
)

set(mayaPluginPath 
    "${CMAKE_INSTALL_PREFIX}/plugin/al/plugin"
    "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/plugin"
)

separate_argument_list(path)
separate_argument_list(pythonPath)
separate_argument_list(mayaPluginPath)

# unit tests
add_test(
    NAME ${TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${MAYA_PY_EXECUTABLE} -c "import sys;from unittest import main;import maya.standalone; \
                                      maya.standalone.initialize(name='python'); \
                                      import testPxrTranslators;testProg=main(module=testPxrTranslators,exit=False); \
                                      maya.standalone.uninitialize(); \
                                      sys.exit(not testProg.result.wasSuccessful());
                                      "
)

set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT 
    "PYTHONPATH=${pythonPath}"
    "PATH=${path}"
    "MAYA_PLUG_IN_PATH=${mayaPluginPath}"
    "MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib/usd/usdMaya/resources"
    "PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/plugin/al/plugin"
    "TEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    "MAYA_NO_STANDALONE_ATEXIT=1"
)

if(IS_LINUX)
    set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT 
        "LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/plugin/al/lib:${PXR_USD_LOCATION}/lib"
    )
endif()
