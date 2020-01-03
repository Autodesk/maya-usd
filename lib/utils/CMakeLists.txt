# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
       colorSpace.cpp
       query.cpp
       stageCache.cpp
       util.cpp
       utilFileSystem.cpp
)

set(headers
    colorSpace.h
    query.h
    query.h
    stageCache.h
    util.h
    utilFileSystem.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(${TARGET_NAME} HEADERS ${headers} DIRNAME utils)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/utils/
)