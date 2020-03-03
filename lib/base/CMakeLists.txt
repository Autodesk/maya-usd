# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        debugCodes.cpp
)

set(headers
    api.h
    debugCodes.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteMayaUsdHeader()
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR base)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/base
)

install(FILES ${CMAKE_BINARY_DIR}/include/mayaUsd/mayaUsd.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd
)
