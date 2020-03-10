mayaUsd_add_test(TestALUSDMayaPython_ProxyShape
    PYTHON_MODULE testProxyShape
)

mayaUsd_add_test(TestALUSDMayaPython_Translators
    PYTHON_MODULE testTranslators
)

mayaUsd_add_test(TestALUSDMayaPython_LayerManager
    PYTHON_MODULE testLayerManager
)

mayaUsd_add_test(TestALUSDMayaPython_ImportModule
    PYTHON_MODULE testUSDMayaPythonModuleOnly
    NO_STANDALONE_INIT
)