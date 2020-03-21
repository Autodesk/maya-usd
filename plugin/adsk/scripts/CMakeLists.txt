list(APPEND scripts_src
    mayaUsdTranslatorImport.mel
    mayaUsdTranslatorExport.mel
    AEmayaUsdProxyShapeTemplate.mel
    mayaUsdMenu.mel
    mayaUsd_createStagesFromExistingLayer.mel
)

install(FILES ${scripts_src}
    DESTINATION ${INSTALL_DIR_SUFFIX}/scripts
)
