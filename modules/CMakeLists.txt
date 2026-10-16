# ---------------------------------------------------------------------------------------------
# install
# ---------------------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# MaterialX Python bindings - variable "MAYAUSD_EXTRA_PYTHON_PATH" is used in mod files.
#------------------------------------------------------------------------------
if (USD_VERSION VERSION_LESS "0.23.11")
    set(MAYAUSD_EXTRA_PYTHON_PATH "")
else()
    set(MAYAUSD_EXTRA_PYTHON_PATH "PYTHONPATH+:=python")
endif()

#------------------------------------------------------------------------------
# USD python bindings - variable "PXR_USD_PYTHON_LOCATION" is used in mod files.
#------------------------------------------------------------------------------
get_pxr_usd_python_module_location(PXR_USD_PYTHON_LOCATION)

if (BUILD_ADSK_PLUGIN)
    if (IS_WINDOWS)
        configure_file("mayaUSD_Win.mod.template" ${PROJECT_BINARY_DIR}/mayaUSD.mod)
    else()
        configure_file("mayaUSD.mod.template" ${PROJECT_BINARY_DIR}/mayaUSD.mod)
    endif()
    install(FILES ${PROJECT_BINARY_DIR}/mayaUSD.mod DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

if (BUILD_HDMAYA)
    configure_file("mtoh.mod.template" ${PROJECT_BINARY_DIR}/mtoh.mod)
    install(FILES ${PROJECT_BINARY_DIR}/mtoh.mod DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

if (BUILD_PXR_PLUGIN)
    if (IS_WINDOWS)
        configure_file("pxrUSD_Win.mod.template" ${PROJECT_BINARY_DIR}/pxrUSD.mod)
    else()
        configure_file("pxrUSD.mod.template" ${PROJECT_BINARY_DIR}/pxrUSD.mod)
    endif()
    install(FILES ${PROJECT_BINARY_DIR}/pxrUSD.mod DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

if (BUILD_AL_PLUGIN)
    if (IS_WINDOWS)
        configure_file("alUSD_Win.mod.template" ${PROJECT_BINARY_DIR}/alUSD.mod)
    else()
        configure_file("alUSD.mod.template" ${PROJECT_BINARY_DIR}/alUSD.mod)
    endif()
    install(FILES ${PROJECT_BINARY_DIR}/alUSD.mod DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
