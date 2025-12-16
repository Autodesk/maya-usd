mayaUsd_add_test(TestALUSDMayaPython_ProxyShape
    PYTHON_MODULE testProxyShape
    ENV
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${ADDITIONAL_PXR_PLUGINPATH_NAME}"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)

mayaUsd_add_test(TestALUSDMayaPython_Translators
    PYTHON_MODULE testTranslators
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${ADDITIONAL_PXR_PLUGINPATH_NAME}"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)

mayaUsd_add_test(TestALUSDMayaPython_LayerManager
    PYTHON_MODULE testLayerManager
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${ADDITIONAL_PXR_PLUGINPATH_NAME}"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)

# In Maya 2024 mayapy was changed to an executable script file which adds
# the lib path the LD_LIBRARY_PATH. For Maya 2023 (and below) we need to
# mimic that here so that tests can find Maya libs. Normally this is
# handled by maya.standalone.initialize() but this test is run no standalone.
if (IS_LINUX AND MAYA_APP_VERSION VERSION_LESS_EQUAL 2023)
    get_filename_component(MAYAPY_EXE_PATH ${MAYA_PY_EXECUTABLE} DIRECTORY)
    set(MAYA_LIB_PATH "${MAYAPY_EXE_PATH}/../lib")
endif()

mayaUsd_add_test(TestALUSDMayaPython_ImportModule
    PYTHON_MODULE testUSDMayaPythonModuleOnly
    ENV
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${ADDITIONAL_PXR_PLUGINPATH_NAME}"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}:${MAYA_LIB_PATH}"
    NO_STANDALONE_INIT
)