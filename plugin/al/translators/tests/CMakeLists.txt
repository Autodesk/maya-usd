mayaUsd_add_test(TestAdditionalTranslators
    PYTHON_MODULE testTranslators
    ENV
        "PXR_PLUGINPATH_NAME=$ENV{PXR_PLUGINPATH_NAME}"
)
