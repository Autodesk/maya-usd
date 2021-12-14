# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        OpUndoItemList.cpp
        OpUndoItemRecorder.cpp
        OpUndoItems.cpp
        UsdUndoBlock.cpp
        UsdUndoManager.cpp
        UsdUndoStateDelegate.cpp
        UsdUndoableItem.cpp

)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
set(HEADERS
    OpUndoItemList.h
    OpUndoItemMuting.h
    OpUndoItemRecorder.h
    OpUndoItems.h
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
