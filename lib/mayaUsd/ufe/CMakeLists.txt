# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        Global.cpp
        MayaStagesSubject.cpp
        MayaUsdContextOps.cpp
        MayaUsdContextOpsHandler.cpp
        MayaUsdObject3d.cpp
        MayaUsdObject3dHandler.cpp
        MayaUsdSceneItemOps.cpp
        MayaUsdSceneItemOpsHandler.cpp
        MayaUsdUIInfoHandler.cpp
        MayaUsdUndoRenameCommand.cpp
        ProxyShapeContextOpsHandler.cpp
        ProxyShapeHandler.cpp
        ProxyShapeHierarchy.cpp
        ProxyShapeHierarchyHandler.cpp
        UsdStageMap.cpp
        UsdUIUfeObserver.cpp
        UsdUndoDuplicateCommand.cpp
        UsdUndoMaterialCommands.cpp
        Utils.cpp
        moduleDeps.cpp
)

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            # See MayaUIInfoHandler.h comments.
            EditAsMayaCommand.cpp
            MayaUIInfoHandler.cpp
            MayaUsdHierarchy.cpp
            MayaUsdHierarchyHandler.cpp
            MayaUsdRootChildHierarchy.cpp
            PulledObjectHierarchy.cpp
            PulledObjectHierarchyHandler.cpp
            UsdPathMappingHandler.cpp
    )
endif()

if (UFE_LIGHTS_SUPPORT)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdLight.cpp
            UsdLightHandler.cpp
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_LIGHTS_SUPPORT=1
    )
    if (UFE_VOLUME_LIGHTS_SUPPORT)
        target_compile_definitions(${PROJECT_NAME}
    	    PRIVATE
                UFE_VOLUME_LIGHTS_SUPPORT=1
        )
    endif()
endif()

if (UFE_LIGHTS2_SUPPORT)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdLight2.cpp
            UsdLight2Handler.cpp
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_LIGHTS2_SUPPORT=1
    )
endif()

if (UFE_MATERIALS_SUPPORT)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdMaterial.cpp
            UsdMaterialHandler.cpp
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_MATERIALS_SUPPORT=1
    )

    if (UFE_MATERIAL_HAS_HASMATERIAL)
        target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            UFE_MATERIAL_HAS_HASMATERIAL=1
        )
    endif()
endif()

if (UFE_SCENE_SEGMENT_SUPPORT)
    target_sources(${PROJECT_NAME}
    PRIVATE
        ProxyShapeSceneSegmentHandler.cpp
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_SCENE_SEGMENT_SUPPORT=1
    )
endif()

if (UFE_UINODEGRAPHNODE_HAS_SIZE)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_UINODEGRAPHNODE_HAS_SIZE=1
    )
endif()

if (UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR=1
    )
endif()

if (UFE_CONTEXTOPS_HAS_OPTIONBOX)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_CONTEXTOPS_HAS_OPTIONBOX=1
    )
endif()

if (v4_BatchOps IN_LIST UFE_PREVIEW_FEATURES)
    message(STATUS "UFE_PREVIEW has V4 BatchOps support")
    target_sources(${PROJECT_NAME}
    PRIVATE
        UsdBatchOpsHandler.cpp
        UsdUndoDuplicateSelectionCommand.cpp
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_PREVIEW_BATCHOPS_SUPPORT=1
    )
endif()

if(CodeWrapperHandler IN_LIST UFE_PREVIEW_FEATURES)
    message(STATUS "UFE_PREVIEW has composite command handler support")
    target_sources(${PROJECT_NAME}
        PRIVATE
        UsdCodeWrapperHandler.cpp
    )

    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        UFE_PREVIEW_CODE_WRAPPER_HANDLER_SUPPORT=1
    )
endif()


if (UFE_SCENE_SEGMENT_HANDLER_ROOT_PATH)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_SCENE_SEGMENT_HANDLER_ROOT_PATH=1
    )
endif()

if (NOT MAYA_APP_VERSION VERSION_GREATER 2025)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        MAYAUSD_NEED_OUTLINER_FILTER_FIX=1
    )
endif()

if (UFE_CAMERAHANDLER_HAS_FINDALL)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_CAMERAHANDLER_HAS_FINDALL=1
    )
endif()

if (UFE_BATCH_OPS_HAS_DUPLICATE_TO_TARGET)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_BATCH_OPS_HAS_DUPLICATE_TO_TARGET=1
    )
endif()

if(UFE_CAMERAHANDLER_HAS_FINDALL)
    target_sources(${PROJECT_NAME}
        PRIVATE
            ProxyShapeCameraHandler.cpp
    )
endif()

if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdShaderNodeDef.cpp
            UsdShaderNodeDefHandler.cpp
            UsdUndoCreateFromNodeDefCommand.cpp
            UsdConnections.cpp
            UsdConnectionHandler.cpp
            UsdUINodeGraphNode.cpp
            UsdUINodeGraphNodeHandler.cpp
            UsdUndoConnectionCommands.cpp
            UsdUndoCreateStageWithNewLayerCommand.cpp
    )
endif()

set(HEADERS
    Global.h
    MayaStagesSubject.h
    MayaUsdContextOps.h
    MayaUsdContextOpsHandler.h
    MayaUsdObject3d.h
    MayaUsdObject3dHandler.h
    MayaUsdSceneItemOps.h
    MayaUsdSceneItemOpsHandler.h
    MayaUsdUIInfoHandler.h
    MayaUsdUndoRenameCommand.h
    ProxyShapeContextOpsHandler.h
    ProxyShapeHandler.h
    ProxyShapeHierarchy.h
    ProxyShapeHierarchyHandler.h
    UsdStageMap.h
    UsdUIUfeObserver.h
    UsdUndoDuplicateCommand.h
    UsdUndoMaterialCommands.h
    Utils.h
)

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        # Not dependent on UFE v3, but used to draw orphaned pulled Maya nodes
        # in the Outliner, which is a UFE v3 feature.
        EditAsMayaCommand.h
        MayaUIInfoHandler.h
        MayaUsdHierarchy.h
        MayaUsdHierarchyHandler.h
        MayaUsdRootChildHierarchy.h
        PulledObjectHierarchy.h
        PulledObjectHierarchyHandler.h
        UsdPathMappingHandler.h
    )
endif()

if (UFE_LIGHTS_SUPPORT)
    list(APPEND HEADERS
        UsdLight.h
        UsdLightHandler.h
    )
endif()

if (UFE_LIGHTS2_SUPPORT)
    list(APPEND HEADERS
        UsdLight2.h
        UsdLight2Handler.h
    )
endif()

if (UFE_MATERIALS_SUPPORT)
    list(APPEND HEADERS
        UsdMaterial.h
        UsdMaterialHandler.h
    )
endif()

if (UFE_SCENE_SEGMENT_SUPPORT)
    list(APPEND HEADERS
        ProxyShapeSceneSegmentHandler.h
    )
endif()

if (v4_BatchOps IN_LIST UFE_PREVIEW_FEATURES)
    list(APPEND HEADERS
        UsdBatchOpsHandler.h
        UsdUndoDuplicateSelectionCommand.h
    )
endif()

if(CodeWrapperHandler IN_LIST UFE_PREVIEW_FEATURES)
    list(APPEND HEADERS
        UsdCodeWrapperHandler.h
    )
endif()

if(UFE_CAMERAHANDLER_HAS_FINDALL)
    list(APPEND HEADERS
        ProxyShapeCameraHandler.h
    )
endif()

if (UFE_CAMERA_HAS_COMPUTEDVISIBILITY)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        UFE_CAMERA_HAS_COMPUTEDVISIBILITY=1
    )
endif()
       
if(CMAKE_UFE_V4_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        UsdShaderNodeDef.h
        UsdShaderNodeDefHandler.h
        UsdUndoCreateFromNodeDefCommand.h
        UsdConnections.h
        UsdConnectionHandler.h
        UsdUINodeGraphNode.h
        UsdUINodeGraphNodeHandler.h
        UsdUndoConnectionCommands.h
        UsdUndoCreateStageWithNewLayerCommand.h
    )
endif()

add_subdirectory(trf)

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
# target (UFE Python Bindings)
# -----------------------------------------------------------------------------
# UFE Python Bindings
if(IS_WINDOWS AND MAYAUSD_DEFINE_DEBUG_PYTHON_FLAG)
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
