# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        UsdUndoBlock.cpp
        UsdUndoManager.cpp
        UsdUndoStateDelegate.cpp
        UsdUndoableItem.cpp
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
set(HEADERS
    ForwardDeclares.h
    UsdUndoBlock.h
    UsdUndoManager.h
    UsdUndoStateDelegate.h
    UsdUndoableItem.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR undo)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/undo/
)
