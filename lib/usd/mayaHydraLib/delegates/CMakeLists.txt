# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        delegate.cpp
        delegateCtx.cpp
        delegateDebugCodes.cpp
        delegateRegistry.cpp
        sceneDelegate.cpp
        testDelegate.cpp
)

set(HEADERS
    delegate.h
    delegateCtx.h
    delegateDebugCodes.h
    delegateRegistry.h
    params.h
    sceneDelegate.h
    testDelegate.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList( 
    HEADERS
        ${HEADERS}
    BASEDIR
        ${TARGET_NAME}/delegates
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/include/mayaHydraLib/delegates
)
