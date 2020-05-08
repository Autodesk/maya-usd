set(TARGET_NAME "mayaUsdUtils")

add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        DebugCodes.cpp
        DiffCore.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        MAYA_USD_UTILS_EXPORT
)

mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME} 
    PUBLIC
        ${CMAKE_BINARY_DIR}/include
    INTERFACE
        ${USD_INCLUDE_DIR}
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PUBLIC
        gf
        usd
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
set(headers
    ALHalf.h
    Api.h
    DebugCodes.h
    DiffCore.h
    ForwardDeclares.h
    SIMD.h
)

mayaUsd_promoteHeaderList( 
    HEADERS
        ${headers}
    BASEDIR
        ${TARGET_NAME}
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS ${TARGET_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(FILES ${headers}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${TARGET_NAME}
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL
    )
endif()
