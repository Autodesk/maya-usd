set(TARGET_NAME hdMaya)

add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
        debugCodes.cpp
        utils.cpp
)

set(HEADERS
    api.h
    utils.h
    debugCodes.h
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME} 
    PRIVATE 
        HDMAYA_EXPORT
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME} 
    PUBLIC 
        mayaUsd
    PRIVATE
        $<$<BOOL:${USD_001905_BUILD}>:hio>
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList( 
    HEADERS
        ${HEADERS}
    BASEDIR
        ${TARGET_NAME}
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
set(LIBFILENAME ${CMAKE_SHARED_LIBRARY_PREFIX}${TARGET_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
set(PLUG_INFO_PATH "plugInfo.json")
set(SHADER_DEFS_PATH 
"shaderDefs.usda"
"mayaStippleShader.glslfx"
)

set(PLUG_INFO_LIBRARY_PATH "../../../${LIBFILENAME}")
set(PLUG_INFO_RESOURCE_PATH "resources")

configure_file(${PLUG_INFO_PATH} ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH})

# configure shader files
foreach(X ${SHADER_DEFS_PATH})
    configure_file(${X} ${CMAKE_CURRENT_BINARY_DIR}/${X})
endforeach(X)


install(TARGETS ${TARGET_NAME}
        LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        RUNTIME
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(FILES 
            ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
        DESTINATION 
            ${CMAKE_INSTALL_PREFIX}/lib/usd/hdMaya/resources
)

# install shaders
foreach(X ${SHADER_DEFS_PATH})
    install(FILES 
            ${CMAKE_CURRENT_BINARY_DIR}/${X}
        DESTINATION 
            ${CMAKE_INSTALL_PREFIX}/lib/usd/hdMaya/resources
    )
endforeach(X)


install(FILES ${HEADERS}
        DESTINATION 
            ${CMAKE_INSTALL_PREFIX}/include/hdMaya
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()

# -----------------------------------------------------------------------------
# subdirectories
# -----------------------------------------------------------------------------
add_subdirectory(adapters)
add_subdirectory(delegates)
add_subdirectory(renderItemClient)
