# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        delegate.cpp
        delegateCtx.cpp
        delegateDebugCodes.cpp
        delegateRegistry.cpp
        proxyDelegate.cpp
        proxyUsdImagingDelegate.cpp
        sceneDelegate.cpp
        testDelegate.cpp
)

set(headers
    delegate.h
    delegateCtx.h
    delegateDebugCodes.h
    delegateRegistry.h
    params.h
    proxyDelegate.h
    proxyUsdImagingDelegate.h
    sceneDelegate.h
    testDelegate.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList( 
    HEADERS
        ${headers}
    BASEDIR
        ${TARGET_NAME}/delegates
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/include/hdMaya/delegates
)