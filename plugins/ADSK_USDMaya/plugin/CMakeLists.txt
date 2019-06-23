set(PLUGIN_PACKAGE mayaUsdPlugin)

add_library(${PLUGIN_PACKAGE} MODULE
    plugin.cpp
    importTranslator.cpp
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
            SUFFIX ".bundle")
else()
    set(rpath "$ORIGIN:$ORIGIN/../lib")
    if (CMAKE_INSTALL_RPATH)
        set(rpath "${CMAKE_INSTALL_RPATH}:${rpath}")
    endif()
    set_target_properties(${PLUGIN_PACKAGE}
        PROPERTIES
            PREFIX ""
            INSTALL_RPATH "${rpath}"
    )
endif()

install(TARGETS
    ${PLUGIN_PACKAGE}
    LIBRARY
        DESTINATION plugin/mayaUsd
    RUNTIME
        DESTINATION plugin/mayaUsd
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${PLUGIN_PACKAGE}> DESTINATION plugin/mayaUSD OPTIONAL)
endif()

add_subdirectory(translators)