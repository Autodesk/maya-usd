list(APPEND scripts_src
    mayaUsdTranslatorImport.mel
    mayaUsdTranslatorExport.mel
)

install(FILES ${scripts_src}
  DESTINATION ${INSTALL_DIR_SUFFIX}/scripts
)
