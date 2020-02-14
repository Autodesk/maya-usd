set(PLUGIN_PACKAGE mayaUsdPlugin)

add_library(${PLUGIN_PACKAGE} MODULE
    plugin.cpp
    importTranslator.cpp
    ProxyShape.cpp
)

target_compile_definitions(${PLUGIN_PACKAGE}
    PRIVATE
        MAYAUSD_PLUGIN_EXPORT
        $<$<BOOL:${Qt5_FOUND}>:WANT_QT_BUILD>
        $<$<BOOL:${UFE_FOUND}>:WANT_UFE_BUILD>
)

target_include_directories(${PLUGIN_PACKAGE}
    PRIVATE
        ${MAYAUSD_INCLUDE_DIR}
        ${PXR_INCLUDE_DIRS}
        $<$<BOOL:${UFE_FOUND}>:${UFE_INCLUDE_DIR}>
)

target_link_libraries(${PLUGIN_PACKAGE}
    PRIVATE
        ${MAYA_LIBRARIES}
        mayaUsd
		ar 
		gf 
		kind
		plug 
		sdf 
		tf
		usd
		usdGeom
		usdUtils
		$<$<BOOL:${Qt5_FOUND}>:mayaUsdUI>
        $<$<BOOL:${UFE_FOUND}>:${UFE_LIBRARY}>
)

MAYA_SET_PLUGIN_PROPERTIES(${PLUGIN_PACKAGE})

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "plugin")
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
    mayaUsd_install_rpath(rpath ${PLUGIN_PACKAGE})
endif()

install(TARGETS
    ${PLUGIN_PACKAGE}
    LIBRARY
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
    RUNTIME
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PLUGIN_PACKAGE}> DESTINATION ${INSTALL_DIR_SUFFIX}/plugin OPTIONAL)
endif()

