set(TARGET_NAME "MayaUsdUtils")

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
# compile definitions
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        MAYA_USD_UTILS_EXPORT
)
 
# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME} 
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${USD_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/include
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

mayaUsd_promoteHeaderListWithSubdir( 
    HEADERS 
        ${headers}
    BASE_PATH ${TARGET_NAME}
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS ${TARGET_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd
)

install(FILES ${headers}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${TARGET_NAME}
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd OPTIONAL
    )
endif()
