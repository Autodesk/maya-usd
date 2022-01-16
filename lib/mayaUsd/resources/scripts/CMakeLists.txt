list(APPEND scripts_src
    mayaUsdAddMayaReference.mel
    mayaUsdAddMayaReference.py
)

install(FILES ${scripts_src}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/scripts
)
