# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        Global.cpp
        ProxyShapeHandler.cpp
        ProxyShapeHierarchy.cpp
        ProxyShapeHierarchyHandler.cpp
        StagesSubject.cpp
        UsdHierarchy.cpp
        UsdHierarchyHandler.cpp
        UsdPointInstanceOrientationModifier.cpp
        UsdPointInstancePositionModifier.cpp
        UsdPointInstanceScaleModifier.cpp
        UsdRootChildHierarchy.cpp
        UsdRotatePivotTranslateUndoableCommand.cpp
        UsdRotateUndoableCommand.cpp
        UsdScaleUndoableCommand.cpp
        UsdSceneItem.cpp
        UsdSceneItemOps.cpp
        UsdSceneItemOpsHandler.cpp
        UsdStageMap.cpp
        UsdTRSUndoableCommandBase.cpp
        UsdTransform3d.cpp
        UsdTransform3dHandler.cpp
        UsdTranslateUndoableCommand.cpp
        UsdUndoDeleteCommand.cpp
        UsdUndoDuplicateCommand.cpp
        UsdUndoRenameCommand.cpp
        Utils.cpp
        moduleDeps.cpp
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            ProxyShapeContextOpsHandler.cpp
            RotationUtils.cpp
            UsdAttribute.cpp
            UsdAttributes.cpp
            UsdAttributesHandler.cpp
            UsdCamera.cpp
            UsdCameraHandler.cpp
            UsdContextOps.cpp
            UsdContextOpsHandler.cpp
            UsdObject3d.cpp
            UsdObject3dHandler.cpp
            UsdSetXformOpUndoableCommandBase.cpp
            UsdTransform3dBase.cpp
            UsdTransform3dCommonAPI.cpp
            UsdTransform3dFallbackMayaXformStack.cpp
            UsdTransform3dMatrixOp.cpp
            UsdTransform3dMayaXformStack.cpp
            UsdTransform3dPointInstance.cpp
            UsdTransform3dSetObjectMatrix.cpp
            UsdTransform3dUndoableCommands.cpp
            UsdUIInfoHandler.cpp
            UsdUIUfeObserver.cpp
            UsdUndoAddNewPrimCommand.cpp
            UsdUndoCreateGroupCommand.cpp
            UsdUndoInsertChildCommand.cpp
            UsdUndoReorderCommand.cpp
            UsdUndoSetKindCommand.cpp
            UsdUndoVisibleCommand.cpp
            UsdUndoUngroupCommand.cpp
            XformOpUtils.cpp
    )
endif()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            PulledObjectHierarchy.cpp
            PulledObjectHierarchyHandler.cpp
            UsdPathMappingHandler.cpp
            UsdUndoUngroupCommand.cpp
    )
endif()

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4004)
        target_sources(${PROJECT_NAME}
            PRIVATE
                UsdLight.cpp
                UsdLightHandler.cpp
        )
    endif()

    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4001)
        target_sources(${PROJECT_NAME}
            PRIVATE
                UsdShaderNodeDef.cpp
                UsdShaderNodeDefHandler.cpp
        )
    endif()
endif()

set(HEADERS
    Global.h
    ProxyShapeHandler.h
    ProxyShapeHierarchy.h
    ProxyShapeHierarchyHandler.h
    StagesSubject.h
    UsdHierarchy.h
    UsdHierarchyHandler.h
    UsdPointInstanceModifierBase.h
    UsdPointInstanceOrientationModifier.h
    UsdPointInstancePositionModifier.h
    UsdPointInstanceScaleModifier.h
    UsdRootChildHierarchy.h
    UsdRotatePivotTranslateUndoableCommand.h
    UsdRotateUndoableCommand.h
    UsdScaleUndoableCommand.h
    UsdSceneItem.h
    UsdSceneItemOps.h
    UsdSceneItemOpsHandler.h
    UsdStageMap.h
    UsdTRSUndoableCommandBase.h
    UsdTransform3d.h
    UsdTransform3dHandler.h
    UsdTranslateUndoableCommand.h
    UsdUndoDeleteCommand.h
    UsdUndoDuplicateCommand.h
    UsdUndoRenameCommand.h
    Utils.h
    UfeVersionCompat.h
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        ProxyShapeContextOpsHandler.h
        RotationUtils.h
        UsdAttribute.h
        UsdAttributes.h
        UsdAttributesHandler.h
        UsdCamera.h
        UsdCameraHandler.h
        UsdContextOps.h
        UsdContextOpsHandler.h
        UsdObject3d.h
        UsdObject3dHandler.h
        UsdPointInstanceUndoableCommands.h
        UsdSetXformOpUndoableCommandBase.h
        UsdTransform3dBase.h
        UsdTransform3dCommonAPI.h
        UsdTransform3dFallbackMayaXformStack.h
        UsdTransform3dMatrixOp.h
        UsdTransform3dMayaXformStack.h
        UsdTransform3dPointInstance.h
        UsdTransform3dSetObjectMatrix.h
        UsdTransform3dUndoableCommands.h
        UsdUIInfoHandler.h
        UsdUIUfeObserver.h
        UsdUndoableCommand.h
        UsdUndoAddNewPrimCommand.h
        UsdUndoCreateGroupCommand.h
        UsdUndoInsertChildCommand.h
        UsdUndoReorderCommand.h
        UsdUndoSetKindCommand.h
        UsdUndoVisibleCommand.h
        XformOpUtils.h
    )
endif()

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        PulledObjectHierarchy.h
        PulledObjectHierarchyHandler.h
        UsdPathMappingHandler.h
        UsdUndoUngroupCommand.h
    )
endif()

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4004)
        list(APPEND HEADERS
            UsdLight.h
            UsdLightHandler.h
        )
    endif()

    if (${UFE_PREVIEW_VERSION_NUM} GREATER_EQUAL 4001)
        list(APPEND HEADERS
            UsdShaderNodeDef.h
            UsdShaderNodeDefHandler.h
        )
    endif()
endif()

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR ufe)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/ufe
)

set(UFE_PYTHON_MODULE_NAME ufe)

# -----------------------------------------------------------------------------
# subdirectories
# -----------------------------------------------------------------------------
add_subdirectory(private)

# -----------------------------------------------------------------------------
# target (UFE Python Bindings)
# -----------------------------------------------------------------------------
# UFE Python Bindings
if(IS_WINDOWS AND MAYAUSD_DEFINE_BOOST_DEBUG_PYTHON_FLAG)
    # On Windows when compiling with debug python the library must be named with _d.
    set(UFE_PYTHON_TARGET_NAME "_${UFE_PYTHON_MODULE_NAME}_d")
else()
    set(UFE_PYTHON_TARGET_NAME "_${UFE_PYTHON_MODULE_NAME}")
endif()

add_library(${UFE_PYTHON_TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${UFE_PYTHON_TARGET_NAME} 
    PRIVATE
        module.cpp
        wrapGlobal.cpp
        wrapUtils.cpp
        wrapNotice.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${UFE_PYTHON_TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        MFB_PACKAGE_NAME=${UFE_PYTHON_MODULE_NAME}
        MFB_ALT_PACKAGE_NAME=${UFE_PYTHON_MODULE_NAME}
        MFB_PACKAGE_MODULE="${PROJECT_NAME}.${UFE_PYTHON_MODULE_NAME}"
)

mayaUsd_compile_config(${UFE_PYTHON_TARGET_NAME})

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${UFE_PYTHON_TARGET_NAME}
    PUBLIC
        ${PROJECT_NAME}
)

# -----------------------------------------------------------------------------
# properties
# -----------------------------------------------------------------------------
set_python_module_property(${UFE_PYTHON_TARGET_NAME})

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "${PROJECT_NAME}")
    mayaUsd_add_rpath(rpath "../../..")
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    endif()
    mayaUsd_install_rpath(rpath ${UFE_PYTHON_TARGET_NAME})
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS ${UFE_PYTHON_TARGET_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/${UFE_PYTHON_MODULE_NAME}
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/${UFE_PYTHON_MODULE_NAME}
)

install(FILES __init__.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/${UFE_PYTHON_MODULE_NAME}
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${UFE_PYTHON_TARGET_NAME}> 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/${UFE_PYTHON_MODULE_NAME} OPTIONAL)
endif()
