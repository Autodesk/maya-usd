set(PLUGIN_PACKAGE mayaUsdPlugin)

if (NOT "${MAYA_DEVKIT_LOCATION}" STREQUAL "")
    find_package(UFE REQUIRED)
    include_directories(${UFE_INCLUDE_DIR})

    # WANT_UFE_BUILD is used in mayaUsdPlugin for #ifdef test.
    add_definitions(-DWANT_UFE_BUILD)
endif()

add_library(${PLUGIN_PACKAGE} MODULE
    plugin.cpp
    importTranslator.cpp
    ProxyShape.cpp
)

target_compile_definitions(${PLUGIN_PACKAGE}
    PRIVATE
        MAYAUSD_PLUGIN_EXPORT
)

target_include_directories(${PLUGIN_PACKAGE}
    PRIVATE
        ${MAYA_INCLUDE_DIRS}
        ${MAYAUSD_INCLUDE_DIR}
        ${PXR_INCLUDE_DIRS}
)

target_link_libraries(${PLUGIN_PACKAGE}
    PRIVATE
        ${MAYA_LIBRARIES}
        ${MAYAUSD_LIBRARIES}
		ar 
		gf 
		kind
		plug 
		sdf 
		tf
		usd
		usdGeom
		usdUtils
)

if(MSVC)
    set_target_properties(${PLUGIN_PACKAGE} PROPERTIES SUFFIX ".mll")
elseif(APPLE)
    set_target_properties(${PLUGIN_PACKAGE}
        PROPERTIES
            PREFIX ""
            SUFFIX ".bundle"
            MACOSX_RPATH TRUE
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "@loader_path/../../../lib"
    )
else()
    set_target_properties(${PLUGIN_PACKAGE}
        PROPERTIES
            PREFIX ""
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "$ORIGIN/../../../lib"
    )
endif()

install(TARGETS
    ${PLUGIN_PACKAGE}
    LIBRARY
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
    RUNTIME
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${PLUGIN_PACKAGE}> DESTINATION ${INSTALL_DIR_SUFFIX}/plugin OPTIONAL)
endif()

