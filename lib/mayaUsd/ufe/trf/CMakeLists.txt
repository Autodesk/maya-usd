# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        RotationUtils.cpp
        UsdPointInstanceOrientationModifier.cpp
        UsdPointInstancePositionModifier.cpp
        UsdPointInstanceScaleModifier.cpp
        UsdSetXformOpUndoableCommandBase.cpp
        UsdTransform3dBase.cpp
        UsdTransform3dCommonAPI.cpp
        UsdTransform3dFallbackMayaXformStack.cpp
        UsdTransform3dMatrixOp.cpp
        UsdTransform3dMayaXformStack.cpp
        UsdTransform3dPointInstance.cpp
        UsdTransform3dReadImpl.cpp
        UsdTransform3dSetObjectMatrix.cpp
        UsdTransform3dUndoableCommands.cpp
        XformOpUtils.cpp
)

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdTransform3dRead.cpp
    )
endif()

set(HEADERS
    RotationUtils.h
    UsdPointInstanceModifierBase.h
    UsdPointInstanceOrientationModifier.h
    UsdPointInstancePositionModifier.h
    UsdPointInstanceScaleModifier.h
    UsdPointInstanceUndoableCommands.h
    UsdSetXformOpUndoableCommandBase.h
    UsdTransform3dBase.h
    UsdTransform3dCommonAPI.h
    UsdTransform3dFallbackMayaXformStack.h
    UsdTransform3dMatrixOp.h
    UsdTransform3dMayaXformStack.h
    UsdTransform3dPointInstance.h
    UsdTransform3dReadImpl.h
    UsdTransform3dSetObjectMatrix.h
    UsdTransform3dUndoableCommands.h
    XformOpUtils.h
)

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        UsdTransform3dRead.h
    )
endif()

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR ufe/trf)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/ufe/trf
)
