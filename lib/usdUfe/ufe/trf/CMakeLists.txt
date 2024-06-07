# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        UsdPointInstanceOrientationModifier.cpp
        UsdPointInstancePositionModifier.cpp
        UsdPointInstanceScaleModifier.cpp
        UsdRotateUndoableCommand.cpp
        UsdScaleUndoableCommand.cpp
        UsdSetXformOpUndoableCommandBase.cpp
        UsdTransform3dBase.cpp
        UsdTransform3dCommonAPI.cpp
        UsdTransform3dMatrixOp.cpp
        UsdTransform3dPointInstance.cpp
        UsdTransform3dReadImpl.cpp
        UsdTransform3dSetObjectMatrix.cpp
        UsdTransform3dUndoableCommands.cpp
        UsdTranslateUndoableCommand.cpp
        UsdTRSUndoableCommandBase.cpp
        Utils.cpp
        XformOpUtils.cpp
)

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdTransform3dRead.cpp
    )
endif()

set(HEADERS
    UsdPointInstanceModifierBase.h
    UsdPointInstanceOrientationModifier.h
    UsdPointInstancePositionModifier.h
    UsdPointInstanceScaleModifier.h
    UsdPointInstanceUndoableCommands.h
    UsdRotateUndoableCommand.h
    UsdScaleUndoableCommand.h
    UsdSetXformOpUndoableCommandBase.h
    UsdTransform3dBase.h
    UsdTransform3dCommonAPI.h
    UsdTransform3dMatrixOp.h
    UsdTransform3dPointInstance.h
    UsdTransform3dReadImpl.h
    UsdTransform3dSetObjectMatrix.h
    UsdTransform3dUndoableCommands.h
    UsdTranslateUndoableCommand.h
    UsdTRSUndoableCommandBase.h
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
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} BASEDIR ${PROJECT_NAME} SUBDIR ufe/trf)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/ufe/trf
)
