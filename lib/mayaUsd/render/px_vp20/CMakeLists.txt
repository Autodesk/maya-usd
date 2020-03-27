# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        glslProgram.cpp
        utils.cpp
        utils_legacy.cpp
)

set(headers
    glslProgram.h
    utils.h
    utils_legacy.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR render/px_vp20)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/render/px_vp20
)
