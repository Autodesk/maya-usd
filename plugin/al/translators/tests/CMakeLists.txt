mayaUsd_add_test(TestAdditionalTranslators
    PYTHON_MODULE testTranslators
    ENV
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${ADDITIONAL_PXR_PLUGINPATH_NAME}"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
