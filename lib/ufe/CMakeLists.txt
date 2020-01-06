# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
        Global.cpp
        ProxyShapeHandler.cpp
        ProxyShapeHierarchy.cpp
        ProxyShapeHierarchyHandler.cpp
        StagesSubject.cpp
        UsdHierarchy.cpp
        UsdHierarchyHandler.cpp
        UsdRootChildHierarchy.cpp
        UsdRotatePivotTranslateUndoableCommand.cpp
        UsdRotateUndoableCommand.cpp
        UsdScaleUndoableCommand.cpp
        UsdSceneItem.cpp
        UsdSceneItemOps.cpp
        UsdSceneItemOpsHandler.cpp
        UsdStageMap.cpp
        UsdTransform3d.cpp
        UsdTransform3dHandler.cpp
        UsdTranslateUndoableCommand.cpp
        UsdUndoDeleteCommand.cpp
        UsdUndoDuplicateCommand.cpp
        UsdUndoRenameCommand.cpp
        Utils.cpp
        private/Utils.cpp
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    target_sources(${TARGET_NAME} 
        PRIVATE
            UsdAttribute.cpp
            UsdAttributes.cpp
            UsdAttributesHandler.cpp
            UsdUndoCreateGroupCommand.cpp
    )
endif()

set(headers
    Global.h
    ProxyShapeHandler.h
    ProxyShapeHierarchy.h
    ProxyShapeHierarchyHandler.h
    StagesSubject.h
    UsdHierarchy.h
    UsdHierarchyHandler.h
    UsdRootChildHierarchy.h
    UsdRotatePivotTranslateUndoableCommand.h
    UsdRotateUndoableCommand.h
    UsdScaleUndoableCommand.h
    UsdSceneItem.h
    UsdSceneItemOps.h
    UsdSceneItemOpsHandler.h
    UsdStageMap.h
    UsdTransform3d.h
    UsdTransform3dHandler.h
    UsdTranslateUndoableCommand.h
    UsdUndoDeleteCommand.h
    UsdUndoDuplicateCommand.h
    UsdUndoRenameCommand.h
    Utils.h
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND headers
        UsdAttribute.h
        UsdAttributes.h
        UsdAttributesHandler.h
        UsdUndoCreateGroupCommand.h
    )
endif()

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME} 
    PUBLIC
        ${UFE_INCLUDE_DIR}
)

# -----------------------------------------------------------------------------
# compile definitions and options
# -----------------------------------------------------------------------------
# some of the UFE classes are exporting STL classes which causes this warning.
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251")
    string(REPLACE " " ";" REPLACED_FLAGS ${CMAKE_CXX_FLAGS})
    target_compile_options(${TARGET_NAME} 
        INTERFACE 
            ${REPLACED_FLAGS}
    )
endif()

target_compile_definitions(${TARGET_NAME} 
    PRIVATE
        WANT_UFE_BUILD=1
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME} 
    PUBLIC
        ${UFE_LIBRARY}
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR ufe)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/ufe
)

#==============================================================================
# target (UFE Python Bindings)
#==============================================================================
set(UFE_PYTHON_TARGET_NAME ufe)

add_library(${UFE_PYTHON_TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${UFE_PYTHON_TARGET_NAME} 
    PRIVATE
    	wrapUtils.cpp
)

# -----------------------------------------------------------------------------
# compile definitions
# -----------------------------------------------------------------------------
target_compile_definitions(${UFE_PYTHON_TARGET_NAME}
    PRIVATE
        MFB_PACKAGE_NAME=${TARGET_NAME}
        MFB_ALT_PACKAGE_NAME=${TARGET_NAME}
        MFB_PACKAGE_MODULE=ufe
)
set_target_properties(${UFE_PYTHON_TARGET_NAME} 
    PROPERTIES 
        PREFIX ""
)
if(IS_WINDOWS)
    set_target_properties(${UFE_PYTHON_TARGET_NAME}
        PROPERTIES
            SUFFIX ".pyd"
    )
elseif(IS_MACOSX)
    set_target_properties(${UFE_PYTHON_TARGET_NAME}
        PROPERTIES
            SUFFIX ".so"
    )
endif()

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${UFE_PYTHON_TARGET_NAME}
    PUBLIC
        ${TARGET_NAME}
)

# handle rpath
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "${PROJECT_NAME}")
    mayaUsd_add_rpath(rpath "../..")
    mayaUsd_install_rpath(rpath ${UFE_PYTHON_TARGET_NAME})
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS ${UFE_PYTHON_TARGET_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${UFE_PYTHON_TARGET_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME} OPTIONAL)
endif()

install(FILES __init__.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}
)

if (CMAKE_UFE_V2_FEATURES_AVAILABLE)
    # We need an empty __init__.py file in each subfolder so it is considered a python module.
    install(FILES ${PROJECT_SOURCE_DIR}/ae/__init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae)
    install(FILES ${PROJECT_SOURCE_DIR}/ae/__init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd)
    install(FILES ${PROJECT_SOURCE_DIR}/ae/__init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd/nodes)

    # Maya Attribute Editor python template files
    foreach(_SUBDIR base camera material mesh scope shader xform)
        install(FILES ${PROJECT_SOURCE_DIR}/ae/__init__.py ${PROJECT_SOURCE_DIR}/ae/${_SUBDIR}/ae_template.py
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd/nodes/${_SUBDIR}
        )
    endforeach()
endif()

