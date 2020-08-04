# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        glslProgram.cpp
        utils.cpp
        utils_legacy.cpp
)

set(HEADERS
    glslProgram.h
    utils.h
    utils_legacy.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR render/px_vp20)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/render/px_vp20
)
