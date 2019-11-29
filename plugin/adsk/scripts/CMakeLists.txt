list(APPEND scripts_src
    AEmayaUsdProxyShapeTemplate.mel
)

install(FILES ${scripts_src}
  DESTINATION ${INSTALL_DIR_SUFFIX}/scripts
)
