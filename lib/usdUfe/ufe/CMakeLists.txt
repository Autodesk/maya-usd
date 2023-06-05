# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PRIVATE
        Global.cpp
        #ProxyShapeHandler.cpp
        #ProxyShapeHierarchy.cpp
        #ProxyShapeHierarchyHandler.cpp
        #SetVariantSelectionCommand.cpp
        #StagesSubject.cpp
        UsdHierarchy.cpp
        UsdHierarchyHandler.cpp
        #UsdPointInstanceOrientationModifier.cpp
        #UsdPointInstancePositionModifier.cpp
        #UsdPointInstanceScaleModifier.cpp
        UsdRootChildHierarchy.cpp
        #UsdRotatePivotTranslateUndoableCommand.cpp
        #UsdRotateUndoableCommand.cpp
        #UsdScaleUndoableCommand.cpp
        UsdSceneItem.cpp
        #UsdSceneItemOps.cpp
        #UsdSceneItemOpsHandler.cpp
        #UsdStageMap.cpp
        #UsdTRSUndoableCommandBase.cpp
        #UsdTransform3d.cpp
        #UsdTransform3dReadImpl.cpp
        #UsdTransform3dHandler.cpp
        #UsdTranslateUndoableCommand.cpp
        #UsdUndoDeleteCommand.cpp
        #UsdUndoDuplicateCommand.cpp
        #UsdUndoRenameCommand.cpp
        Utils.cpp
        #moduleDeps.cpp
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            #ProxyShapeContextOpsHandler.cpp
            #RotationUtils.cpp
            #UsdAttribute.cpp
            #UsdAttributeHolder.cpp
            #UsdAttributes.cpp
            #UsdAttributesHandler.cpp
            #UsdCamera.cpp
            #UsdCameraHandler.cpp
            #UsdContextOps.cpp
            #UsdContextOpsHandler.cpp
            #UsdObject3d.cpp
            #UsdObject3dHandler.cpp
            #UsdSetXformOpUndoableCommandBase.cpp
            #UsdTransform3dBase.cpp
            #UsdTransform3dCommonAPI.cpp
            #UsdTransform3dFallbackMayaXformStack.cpp
            #UsdTransform3dMatrixOp.cpp
            #UsdTransform3dMayaXformStack.cpp
            #UsdTransform3dPointInstance.cpp
            #UsdTransform3dSetObjectMatrix.cpp
            #UsdTransform3dUndoableCommands.cpp
            #UsdUIInfoHandler.cpp
            #UsdUIUfeObserver.cpp
            UsdUndoableCommand.cpp
            UsdUndoAddNewPrimCommand.cpp
            UsdUndoCreateGroupCommand.cpp
            UsdUndoInsertChildCommand.cpp
            UsdUndoReorderCommand.cpp
            UsdUndoSetKindCommand.cpp
            #UsdUndoVisibleCommand.cpp
            #XformOpUtils.cpp
    )
endif()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            # See MayaUIInfoHandler.h comments.
            #EditAsMayaCommand.cpp
            #MayaUIInfoHandler.cpp
            #PulledObjectHierarchy.cpp
            #PulledObjectHierarchyHandler.cpp
            #UsdPathMappingHandler.cpp
            UsdUndoUngroupCommand.cpp
    )
endif()

#if (UFE_LIGHTS_SUPPORT)
#    target_sources(${PROJECT_NAME}
#        PRIVATE
#            #UsdLight.cpp
#            #UsdLightHandler.cpp
#    )
#
#    target_compile_definitions(${PROJECT_NAME}
#    PRIVATE
#        UFE_LIGHTS_SUPPORT=1
#    )
#endif()

#if (UFE_MATERIALS_SUPPORT)
#    target_sources(${PROJECT_NAME}
#        PRIVATE
#            #UsdMaterial.cpp
#            #UsdMaterialHandler.cpp
#    )
#
#    target_compile_definitions(${PROJECT_NAME}
#    PRIVATE
#        UFE_MATERIALS_SUPPORT=1
#    )
#endif()

#if (UFE_SCENE_SEGMENT_SUPPORT)
#    target_sources(${PROJECT_NAME}
#    PRIVATE
#        #ProxyShapeSceneSegmentHandler.cpp
#    )
#
#    target_compile_definitions(${PROJECT_NAME}
#    PRIVATE
#        UFE_SCENE_SEGMENT_SUPPORT=1
#    )
#endif()

#if (v4_BatchOps IN_LIST UFE_PREVIEW_FEATURES)
#    message(STATUS "UFE_PREVIEW has V4 BatchOps support")
#    target_sources(${PROJECT_NAME}
#    PRIVATE
#        #UsdBatchOpsHandler.cpp
#        #UsdUndoDuplicateSelectionCommand.cpp
#    )
#
#    target_compile_definitions(${PROJECT_NAME}
#    PRIVATE
#        UFE_PREVIEW_BATCHOPS_SUPPORT=1
#    )
#endif()

#if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4001)
#        target_sources(${PROJECT_NAME}
#            PRIVATE
#                #UsdShaderNodeDef.cpp
#                #UsdShaderNodeDefHandler.cpp
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4010)
#        target_sources(${PROJECT_NAME}
#            PRIVATE
#                #UsdShaderAttributeDef.cpp
#                #UsdShaderAttributeHolder.cpp
#                #UsdUndoCreateFromNodeDefCommand.cpp
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4013)
#        target_sources(${PROJECT_NAME}
#            PRIVATE
#                #ProxyShapeCameraHandler.cpp
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4020)
#        target_sources(${PROJECT_NAME}
#            PRIVATE
#                #UsdConnections.cpp
#                #UsdConnectionHandler.cpp
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4023)
#        target_sources(${PROJECT_NAME}
#            PRIVATE
#                #UsdUINodeGraphNode.cpp
#                #UsdUINodeGraphNodeHandler.cpp
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4024)
#        target_sources(${PROJECT_NAME}
#            PRIVATE
#                #UsdUndoAttributesCommands.cpp
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4025)
#        target_sources(${PROJECT_NAME}
#            PRIVATE
#                #UsdTransform3dRead.cpp
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4043)
#        target_sources(${PROJECT_NAME}
#            PRIVATE
#                #UsdUndoConnectionCommands.cpp
#        )
#    endif()
#endif()

#if(PXR_VERSION GREATER_EQUAL 2108 AND CMAKE_UFE_V2_FEATURES_AVAILABLE)
#    target_sources(${PROJECT_NAME}
#        PRIVATE
#            #UsdUndoMaterialCommands.cpp
#    )
#endif()

set(HEADERS
    Global.h
    #ProxyShapeHandler.h
    #ProxyShapeHierarchy.h
    #ProxyShapeHierarchyHandler.h
    #SetVariantSelectionCommand.h
    #StagesSubject.h
    UsdHierarchy.h
    UsdHierarchyHandler.h
    #UsdPointInstanceModifierBase.h
    #UsdPointInstanceOrientationModifier.h
    #UsdPointInstancePositionModifier.h
    #UsdPointInstanceScaleModifier.h
    UsdRootChildHierarchy.h
    #UsdRotatePivotTranslateUndoableCommand.h
    #UsdRotateUndoableCommand.h
    #UsdScaleUndoableCommand.h
    UsdSceneItem.h
    #UsdSceneItemOps.h
    #UsdSceneItemOpsHandler.h
    #UsdStageMap.h
    #UsdTRSUndoableCommandBase.h
    #UsdTransform3d.h
    #UsdTransform3dReadImpl.h
    #UsdTransform3dHandler.h
    #UsdTranslateUndoableCommand.h
    #UsdUndoDeleteCommand.h
    #UsdUndoDuplicateCommand.h
    #UsdUndoRenameCommand.h
    Utils.h
    UfeVersionCompat.h
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        #ProxyShapeContextOpsHandler.h
        #RotationUtils.h
        #UsdAttribute.h
        #UsdAttributeHolder.h
        #UsdAttributes.h
        #UsdAttributesHandler.h
        #UsdCamera.h
        #UsdCameraHandler.h
        #UsdContextOps.h
        #UsdContextOpsHandler.h
        #UsdObject3d.h
        #UsdObject3dHandler.h
        #UsdPointInstanceUndoableCommands.h
        #UsdSetXformOpUndoableCommandBase.h
        #UsdTransform3dBase.h
        #UsdTransform3dCommonAPI.h
        #UsdTransform3dFallbackMayaXformStack.h
        #UsdTransform3dMatrixOp.h
        #UsdTransform3dMayaXformStack.h
        #UsdTransform3dPointInstance.h
        #UsdTransform3dSetObjectMatrix.h
        #UsdTransform3dUndoableCommands.h
        #UsdUIInfoHandler.h
        #UsdUIUfeObserver.h
        UsdUndoableCommand.h
        UsdUndoAddNewPrimCommand.h
        UsdUndoCreateGroupCommand.h
        UsdUndoInsertChildCommand.h
        UsdUndoReorderCommand.h
        UsdUndoSetKindCommand.h
        #UsdUndoVisibleCommand.h
        #XformOpUtils.h
    )
endif()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        # Not dependent on UFE v3, but used to draw orphaned pulled Maya nodes
        # in the Outliner, which is a UFE v3 feature.
        #EditAsMayaCommand.h
        #MayaUIInfoHandler.h
        #PulledObjectHierarchy.h
        #PulledObjectHierarchyHandler.h
        #UsdPathMappingHandler.h
        UsdUndoUngroupCommand.h
    )
endif()

#if (UFE_LIGHTS_SUPPORT)
#    list(APPEND HEADERS
#        #UsdLight.h
#        #UsdLightHandler.h
#    )
#endif()

#if (UFE_MATERIALS_SUPPORT)
#    list(APPEND HEADERS
#        #UsdMaterial.h
#        #UsdMaterialHandler.h
#    )
#endif()

#if (UFE_SCENE_SEGMENT_SUPPORT)
#    list(APPEND HEADERS
#        #ProxyShapeSceneSegmentHandler.h
#    )
#endif()

#if (v4_BatchOps IN_LIST UFE_PREVIEW_FEATURES)
#    list(APPEND HEADERS
#        #UsdBatchOpsHandler.h
#        #UsdUndoDuplicateSelectionCommand.h
#    )
#endif()

#if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4001)
#        list(APPEND HEADERS
#            #UsdShaderNodeDef.h
#            #UsdShaderNodeDefHandler.h
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4010)
#        list(APPEND HEADERS
#            #UsdShaderAttributeDef.h
#            #UsdShaderAttributeHolder.h
#            #UsdUndoCreateFromNodeDefCommand.h
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4013)
#        list(APPEND HEADERS
#            #ProxyShapeCameraHandler.h
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4020)
#        list(APPEND HEADERS
#            #UsdConnections.h
#            #UsdConnectionHandler.h
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4023)
#        list(APPEND HEADERS
#            #UsdUINodeGraphNode.h
#            #UsdUINodeGraphNodeHandler.h
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4024)
#        list(APPEND HEADERS
#            #UsdUndoAttributesCommands.h
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4025)
#        list(APPEND HEADERS
#            #UsdTransform3dRead.h
#        )
#    endif()

#    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4043)
#        list(APPEND HEADERS
#            #UsdUndoConnectionCommands.h
#        )
#    endif()
#endif()

#if(PXR_VERSION GREATER_EQUAL 2108 AND CMAKE_UFE_V2_FEATURES_AVAILABLE)
#    list(APPEND HEADERS
#        #UsdUndoMaterialCommands.h
#    )
#endif()

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
