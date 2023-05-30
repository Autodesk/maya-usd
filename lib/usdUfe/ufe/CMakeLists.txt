# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PRIVATE
        Global.cpp
        UsdHierarchy.cpp
        UsdHierarchyHandler.cpp
        UsdRootChildHierarchy.cpp
        UsdSceneItem.cpp
        Utils.cpp
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            UsdUndoableCommand.cpp
            UsdUndoAddNewPrimCommand.cpp
            UsdUndoCreateGroupCommand.cpp
            UsdUndoInsertChildCommand.cpp
            UsdUndoReorderCommand.cpp
            UsdUndoSetKindCommand.cpp
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
    UsdHierarchy.h
    UsdHierarchyHandler.h
    UsdRootChildHierarchy.h
    UsdSceneItem.h
    Utils.h
    UfeVersionCompat.h
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        UsdUndoableCommand.h
        UsdUndoAddNewPrimCommand.h
        UsdUndoCreateGroupCommand.h
        UsdUndoInsertChildCommand.h
        UsdUndoReorderCommand.h
        UsdUndoSetKindCommand.h
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
