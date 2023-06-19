# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PRIVATE
        Global.cpp
        StagesSubject.cpp
        UsdHierarchy.cpp
        UsdHierarchyHandler.cpp
        UsdRootChildHierarchy.cpp
        UsdSceneItem.cpp
        Utils.cpp
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdCamera.cpp
            UsdCameraHandler.cpp
            UsdObject3d.cpp
            UsdObject3dHandler.cpp
            UsdUndoableCommand.cpp
            UsdUndoAddNewPrimCommand.cpp
            UsdUndoAddPayloadCommand.cpp
            UsdUndoAddReferenceCommand.cpp
            UsdUndoClearPayloadsCommand.cpp
            UsdUndoClearReferencesCommand.cpp
            UsdUndoCreateGroupCommand.cpp
            UsdUndoInsertChildCommand.cpp
            UsdUndoReorderCommand.cpp
            UsdUndoSetKindCommand.cpp
            UsdUndoToggleActiveCommand.cpp
            UsdUndoToggleInstanceableCommand.cpp
            UsdUndoVisibleCommand.cpp
    )
endif()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdUndoUngroupCommand.cpp
    )
endif()


set(HEADERS
    Global.h
    StagesSubject.h
    UsdHierarchy.h
    UsdHierarchyHandler.h
    UsdRootChildHierarchy.h
    UsdSceneItem.h
    Utils.h
    UfeVersionCompat.h
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        UsdCamera.h
        UsdCameraHandler.h
        UsdObject3d.h
        UsdObject3dHandler.h
        UsdUndoableCommand.h
        UsdUndoAddNewPrimCommand.h
        UsdUndoAddPayloadCommand.h
        UsdUndoAddReferenceCommand.h
        UsdUndoClearPayloadsCommand.h
        UsdUndoClearReferencesCommand.h
        UsdUndoCreateGroupCommand.h
        UsdUndoInsertChildCommand.h
        UsdUndoReorderCommand.h
        UsdUndoSelectAfterCommand.h
        UsdUndoSetKindCommand.h
        UsdUndoToggleActiveCommand.h
        UsdUndoToggleInstanceableCommand.h
        UsdUndoVisibleCommand.h
    )
endif()

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
