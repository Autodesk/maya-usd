# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------

target_sources(${PROJECT_NAME}
    PRIVATE
        Global.cpp
        SetVariantSelectionCommand.cpp
        StagesSubject.cpp
        UfeNotifGuard.cpp
        UsdAttribute.cpp
        UsdAttributeHolder.cpp
        UsdAttributes.cpp
        UsdAttributesHandler.cpp
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
        UsdSceneItemOps.cpp
        UsdSceneItemOpsHandler.cpp
        UsdUIInfoHandler.cpp
        UsdUndoAddNewPrimCommand.cpp
        UsdUndoAddPayloadCommand.cpp
        UsdUndoAddRefOrPayloadCommand.cpp
        UsdUndoAddReferenceCommand.cpp
        UsdUndoClearPayloadsCommand.cpp
        UsdUndoClearReferencesCommand.cpp
        UsdUndoCreateGroupCommand.cpp
        UsdUndoDeleteCommand.cpp
        UsdUndoDuplicateCommand.cpp
        UsdUndoDuplicateSelectionCommand.cpp
        UsdUndoInsertChildCommand.cpp
        UsdUndoLongDurationCommand.cpp
        UsdUndoPayloadCommand.cpp
        UsdUndoReloadRefCommand.cpp
        UsdUndoRenameCommand.cpp
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

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdShaderAttributeDef.cpp
            UsdShaderAttributeHolder.cpp
            UsdUndoAttributesCommands.cpp
    )
endif()

if (UFE_SCENEITEM_HAS_METADATA)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdUndoClearSceneItemMetadataCommand.cpp
            UsdUndoSetSceneItemMetadataCommand.cpp
    )
endif()

if (UFE_CAMERA_HAS_RENDERABLE)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_CAMERA_HAS_RENDERABLE=1
    )
endif()

if (UFE_CAMERA_HAS_COMPUTEDVISIBILITY)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_CAMERA_HAS_COMPUTEDVISIBILITY=1
    )
endif()

if (UFE_CLIPBOARD_SUPPORT)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdClipboard.cpp
            UsdClipboardCommands.cpp
            UsdClipboardHandler.cpp
    )

    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            UFE_CLIPBOARD_SUPPORT=1
    )
endif()

if (UFE_ATTRIBUTES_GET_ENUMS)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_ATTRIBUTES_GET_ENUMS=1
    )
endif()

if (UFE_HAS_NATIVE_TYPE_METADATA)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_HAS_NATIVE_TYPE_METADATA=1
    )
endif()

if (UFE_CAMERAHANDLER_HAS_FINDALL)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_CAMERAHANDLER_HAS_FINDALL=1
    )
endif()

if (UFE_HAS_UNSIGNED_INT)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_HAS_UNSIGNED_INT=1
    )
endif()

if (UFE_HAS_COLOR_MANAGEMENT_HANDLER)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_HAS_COLOR_MANAGEMENT_HANDLER=1
    )
endif()

set(HEADERS
    Global.h
    SetVariantSelectionCommand.h
    StagesSubject.h
    UfeNotifGuard.h
    UfeVersionCompat.h
    UsdAttribute.h
    UsdAttributeHolder.h
    UsdAttributes.h
    UsdAttributesHandler.h
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
    UsdSceneItemOps.h
    UsdSceneItemOpsHandler.h
    UsdUIInfoHandler.h
    UsdUndoAddNewPrimCommand.h
    UsdUndoAddPayloadCommand.h
    UsdUndoAddRefOrPayloadCommand.h
    UsdUndoAddReferenceCommand.h
    UsdUndoClearPayloadsCommand.h
    UsdUndoClearReferencesCommand.h
    UsdUndoCreateGroupCommand.h
    UsdUndoDeleteCommand.h
    UsdUndoDuplicateCommand.h
    UsdUndoDuplicateSelectionCommand.h
    UsdUndoInsertChildCommand.h
    UsdUndoLongDurationCommand.h
    UsdUndoPayloadCommand.h
    UsdUndoReloadRefCommand.h
    UsdUndoRenameCommand.h
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

if (UFE_SCENEITEM_HAS_METADATA)
    list(APPEND HEADERS
		UsdUndoClearSceneItemMetadataCommand.h
		UsdUndoSetSceneItemMetadataCommand.h
    )
endif()

if (UFE_CLIPBOARD_SUPPORT)
    list(APPEND HEADERS
        UsdClipboard.h
        UsdClipboardCommands.h
        UsdClipboardHandler.h
    )
endif()

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        UsdShaderAttributeDef.h
        UsdShaderAttributeHolder.h
        UsdUndoAttributesCommands.h
    )
endif()

add_subdirectory(trf)

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
