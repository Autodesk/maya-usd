# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        RotationUtils.cpp
        UsdTransform3dFallbackMayaXformStack.cpp
        UsdTransform3dMayaXformStack.cpp
        XformOpUtils.cpp
)

set(HEADERS
    RotationUtils.h
    UsdTransform3dFallbackMayaXformStack.h
    UsdTransform3dMayaXformStack.h
    XformOpUtils.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} BASEDIR ${PROJECT_NAME} SUBDIR ufe/trf)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/ufe/trf
)
