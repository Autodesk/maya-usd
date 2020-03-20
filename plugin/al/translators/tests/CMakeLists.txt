set(TEST_NAME TestAdditionalTranslators)

set(path 
    "${CMAKE_INSTALL_PREFIX}/lib"
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib"
    "$ENV{PATH}"
)

set(pythonPath 
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/python"
    "$ENV{PYTHONPATH}"
)

set(mayaPluginPath 
    "${CMAKE_INSTALL_PREFIX}/plugin/al/plugin"
)

set(pxrPluginPath 
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/usd"
    "${CMAKE_INSTALL_PREFIX}/lib/usd"
)

separate_argument_list(path)
separate_argument_list(pythonPath)
separate_argument_list(mayaPluginPath)
separate_argument_list(pxrPluginPath)

add_test(
    NAME ${TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${MAYA_PY_EXECUTABLE} -c "import sys;from unittest import main;import maya.standalone; \
                                      maya.standalone.initialize(name='python'); \
                                      import testTranslators;testProg=main(module=testTranslators,exit=False); \
                                      maya.standalone.uninitialize(); \
                                      sys.exit(not testProg.result.wasSuccessful());
                                      "
)

set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT 
    "PYTHONPATH=${pythonPath}"
    "PATH=${path}"
    "MAYA_PLUG_IN_PATH=${mayaPluginPath}"
    "PXR_PLUGINPATH_NAME=${pxrPluginPath}"
    "MAYA_NO_STANDALONE_ATEXIT=1"
)

if(IS_LINUX)
    set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT 
        "LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/plugin/al/lib:${PXR_USD_LOCATION}/lib"
    )
endif()
