list(APPEND scripts_src
    mayaUsdLibRegisterStrings.mel
    mayaUsdLibRegisterStrings.py
    mayaUsdAddMayaReference.mel
    mayaUsdAddMayaReference.py
)

install(FILES ${scripts_src}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/scripts
)
