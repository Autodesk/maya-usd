set(MAIN_TEST_NAME TestUSDMayaPython)

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
    NAME ${MAIN_TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${MAYA_PY_EXECUTABLE} -c "import sys;from unittest import main;import maya.standalone; \
                                      maya.standalone.initialize(name='python'); \
                                      import testSuite;main(module=testSuite,exit=False); \
                                      maya.standalone.uninitialize(); \
                                      "
)

set_property(TEST ${MAIN_TEST_NAME} APPEND PROPERTY ENVIRONMENT
    "PYTHONPATH=${pythonPath}"
    "PATH=${path}"
    "MAYA_PLUG_IN_PATH=${mayaPluginPath}"
    "PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/lib/usd"
    "MAYA_NO_STANDALONE_ATEXIT=1"
)

set(MODULE_ONLY_TEST_NAME TestUSDMayaPythonModuleOnly)

add_test(
    NAME ${MODULE_ONLY_TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    # Note that, since this test does NOT call maya.standalone.initialize, we can
    # just let unittest.main call sys.exit "normally"
    COMMAND ${MAYA_PY_EXECUTABLE} -c "import sys;from unittest import main; \
                                      import testUSDMayaPythonModuleOnly; main(module=testUSDMayaPythonModuleOnly); \
                                      "
)

set_property(TEST ${MODULE_ONLY_TEST_NAME} APPEND PROPERTY ENVIRONMENT
    "PYTHONPATH=${pythonPath}"
    "PATH=${path}"
    "MAYA_PLUG_IN_PATH=${mayaPluginPath}"
    "PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/lib/usd"
    "MAYA_NO_STANDALONE_ATEXIT=1"
)