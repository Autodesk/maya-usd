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

if(IS_WINDOWS)
    string(REPLACE ";" "\;" pythonPath "${pythonPath}")
    string(REPLACE ";" "\;" path "${path}")
    string(REPLACE ";" "\;" mayaPluginPath "${mayaPluginPath}")
else()
    separate_arguments(pythonPath NATIVE_COMMAND "${pythonPath}")
    separate_arguments(path NATIVE_COMMAND "${path}")
    separate_arguments(mayaPluginPath NATIVE_COMMAND "${mayaPluginPath}")

    string(REPLACE "\;" ":" pythonPath "${pythonPath}")
    string(REPLACE "\;" ":" path "${path}")
    string(REPLACE "\;" ":" mayaPluginPath "${mayaPluginPath}")
endif()

add_test(
    NAME ${TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${MAYA_PY_EXECUTABLE} -c "from unittest import main;import maya.standalone; \
                                      maya.standalone.initialize(name='python'); \
                                      import testProxyShape;main(module=testProxyShape); \
                                      import testTranslators;main(module=testTranslators); \
                                      import testLayerManager;main(module=testLayerManager); \
                                      maya.standalone.uninitialize();"
)

set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT 
    "PYTHONPATH=${pythonPath}"
    "PATH=${path}"
    "MAYA_PLUG_IN_PATH=${mayaPluginPath}"
    "PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/lib/usd"
    "MAYA_NO_STANDALONE_ATEXIT=1"
)
