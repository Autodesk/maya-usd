set(PLUGIN_PACKAGE mayaUsdPlugin)

add_library(${PLUGIN_PACKAGE} MODULE
    plugin.cpp
)

target_compile_definitions(${PLUGIN_PACKAGE}
    PRIVATE
        MAYAUSD_PLUGIN_EXPORT
)

target_include_directories(${PLUGIN_PACKAGE}
    PRIVATE
        ${MAYA_INCLUDE_DIRS}
        ${MAYAUSD_INCLUDE_DIR}
)

target_link_libraries(${PLUGIN_PACKAGE}
    PRIVATE
        ${MAYA_LIBRARIES}
        ${MAYAUSD_LIBRARIES}
)

if(MSVC)
    set_target_properties(${PLUGIN_PACKAGE} PROPERTIES SUFFIX ".mll")
elseif(APPLE)
    set_target_properties(${PLUGIN_PACKAGE} PROPERTIES SUFFIX ".bundle")
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
        DESTINATION plugin
    RUNTIME
        DESTINATION plugin
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${PLUGIN_PACKAGE}> DESTINATION plugin OPTIONAL)
endif()
