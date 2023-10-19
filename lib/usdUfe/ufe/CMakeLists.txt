# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PRIVATE
        Global.cpp
        SetVariantSelectionCommand.cpp
        StagesSubject.cpp
        UsdCamera.cpp
        UsdCameraHandler.cpp
        UsdContextOps.cpp
        UsdContextOpsHandler.cpp
        UsdHierarchy.cpp
        UsdHierarchyHandler.cpp
        UsdObject3d.cpp
        UsdObject3dHandler.cpp
        UsdRootChildHierarchy.cpp
        UsdSceneItem.cpp
        UsdUIInfoHandler.cpp
        UsdUndoAddNewPrimCommand.cpp
        UsdUndoAddPayloadCommand.cpp
        UsdUndoAddRefOrPayloadCommand.cpp
        UsdUndoAddReferenceCommand.cpp
        UsdUndoClearPayloadsCommand.cpp
        UsdUndoClearReferencesCommand.cpp
        UsdUndoCreateGroupCommand.cpp
        UsdUndoInsertChildCommand.cpp
        UsdUndoPayloadCommand.cpp
        UsdUndoReorderCommand.cpp
        UsdUndoSelectAfterCommand.cpp
        UsdUndoSetKindCommand.cpp
        UsdUndoToggleActiveCommand.cpp
        UsdUndoToggleInstanceableCommand.cpp
        UsdUndoVisibleCommand.cpp
        UsdUndoSetDefaultPrimCommand.cpp
        UsdUndoClearDefaultPrimCommand.cpp
        UsdUndoableCommand.cpp
        Utils.cpp
)

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdUndoUngroupCommand.cpp
    )
endif()


set(HEADERS
    Global.h
    SetVariantSelectionCommand.h
    StagesSubject.h
    UfeVersionCompat.h
    UsdCamera.h
    UsdCameraHandler.h
    UsdContextOps.h
    UsdContextOpsHandler.h
    UsdHierarchy.h
    UsdHierarchyHandler.h
    UsdObject3d.h
    UsdObject3dHandler.h
    UsdRootChildHierarchy.h
    UsdSceneItem.h
    UsdUIInfoHandler.h
    UsdUndoAddNewPrimCommand.h
    UsdUndoAddPayloadCommand.h
    UsdUndoAddRefOrPayloadCommand.h
    UsdUndoAddReferenceCommand.h
    UsdUndoClearPayloadsCommand.h
    UsdUndoClearReferencesCommand.h
    UsdUndoCreateGroupCommand.h
    UsdUndoInsertChildCommand.h
    UsdUndoPayloadCommand.h
    UsdUndoReorderCommand.h
    UsdUndoSelectAfterCommand.h
    UsdUndoSetKindCommand.h
    UsdUndoToggleActiveCommand.h
    UsdUndoToggleInstanceableCommand.h
    UsdUndoVisibleCommand.h
    UsdUndoSetDefaultPrimCommand.h
    UsdUndoClearDefaultPrimCommand.h
    UsdUndoableCommand.h
    Utils.h
)

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        UsdUndoUngroupCommand.h
    )
endif()

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} BASEDIR ${PROJECT_NAME} SUBDIR ufe)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/ufe
)

# -----------------------------------------------------------------------------
# subdirectories
# -----------------------------------------------------------------------------
add_subdirectory(private)
