set(PXR_PACKAGE AL_USDMayaPlugin)

add_library(
    ${PXR_PACKAGE}
    plugin.cpp
)

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
    AL_USDUtils
    AL_USDMaya
    ${MAYA_LIBRARIES}
)

target_include_directories(${PXR_PACKAGE}
    PRIVATE
        ${MAYA_INCLUDE_DIRS}
        ${MAYAUSD_INCLUDE_DIR}
        "../../lib/AL_USDMaya"
)

MAYA_SET_PLUGIN_PROPERTIES(${PXR_PACKAGE})

# rpath setup
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "plugin")
    mayaUsd_add_rpath(rpath "../lib")
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib")
    mayaUsd_install_rpath(rpath ${PXR_PACKAGE})
endif()

if(IS_LINUX)
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../USD/lib64")
    endif()
endif()

install(TARGETS
    ${PXR_PACKAGE}
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PXR_PACKAGE}> DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin OPTIONAL)
endif()
