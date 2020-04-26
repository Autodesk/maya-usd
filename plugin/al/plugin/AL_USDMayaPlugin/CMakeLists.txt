set(PXR_PACKAGE AL_USDMayaPlugin)

add_library(
    ${PXR_PACKAGE}
    plugin.cpp
)

# compiler configuration
mayaUsd_compile_config(${PXR_PACKAGE})

target_compile_definitions(${PXR_PACKAGE}
    PRIVATE
        AL_USDMAYA_PLUGIN_EXPORT
)

target_link_libraries(${PXR_PACKAGE}
    ar
    arch
    usdImaging
    usdImagingGL
    AL_EventSystem
    AL_USDMayaUtils
    AL_MayaUtils
    mayaUsdUtils
    AL_USDMaya
    ${MAYA_LIBRARIES}
)

target_include_directories(${PXR_PACKAGE}
    PRIVATE
        ${MAYA_INCLUDE_DIRS}
        "../../lib/AL_USDMaya"
)

maya_set_plugin_properties(${PXR_PACKAGE})

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "plugin")
    mayaUsd_add_rpath(rpath "../lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib")
    mayaUsd_install_rpath(rpath ${PXR_PACKAGE})
endif()

install(TARGETS
    ${PXR_PACKAGE}
    LIBRARY
        DESTINATION ${AL_INSTALL_PREFIX}/plugin
    RUNTIME
        DESTINATION ${AL_INSTALL_PREFIX}/plugin
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PXR_PACKAGE}> DESTINATION ${AL_INSTALL_PREFIX}/plugin OPTIONAL)
endif()
