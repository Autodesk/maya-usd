# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
    registration.cpp
    mayaHydraSceneIndex.cpp
    mayaHydraDataSource.cpp
    mayaHydraPrimvarDataSource.cpp
    mayaHydraDisplayStyleDataSource.cpp
    mayaHydraCameraDataSource.cpp
    mayaHydraLightDataSource.cpp
)

set(HEADERS
    registration.h
    mayaHydraSceneIndex.h
    mayaHydraDataSource.h
    mayaHydraPrimvarDataSource.h
    mayaHydraDisplayStyleDataSource.h
    mayaHydraCameraDataSource.h
    mayaHydraLightDataSource.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList( 
    HEADERS
        ${HEADERS}
    BASEDIR
        ${TARGET_NAME}/sceneIndex
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/include/mayaHydraLib/sceneIndex
)
