set(TEST_NAME TestUSDMayaPython)

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

separate_argument_list(path)
separate_argument_list(pythonPath)
separate_argument_list(mayaPluginPath)

add_test(
    NAME ${TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${MAYA_PY_EXECUTABLE} -c "import sys;from unittest import main;import maya.standalone; \
                                      maya.standalone.initialize(name='python'); \
                                      import testProxyShape; testProg1 = main(module=testProxyShape,exit=False); \
                                      import testTranslators; testProg2 = main(module=testTranslators,exit=False); \
                                      import testLayerManager; testProg3 = main(module=testLayerManager,exit=False); \
                                      maya.standalone.uninitialize(); \
                                      testRes1Failed = not testProg1.result.wasSuccessful(); \
                                      testRes2Failed = not testProg2.result.wasSuccessful(); \
                                      testRes3Failed = not testProg3.result.wasSuccessful(); \
                                      sys.exit(testRes1Failed or testRes2Failed or testRes3Failed); \
                                      "
)

set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT 
    "PYTHONPATH=${pythonPath}"
    "PATH=${path}"
    "MAYA_PLUG_IN_PATH=${mayaPluginPath}"
    "PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/lib/usd"
    "MAYA_NO_STANDALONE_ATEXIT=1"
)
