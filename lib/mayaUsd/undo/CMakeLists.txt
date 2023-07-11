# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        OpUndoItemList.cpp
        OpUndoItemMuting.cpp
        OpUndoItemRecorder.cpp
        OpUndoItemValidator.cpp
        OpUndoItems.cpp
        MayaUsdUndoBlock.cpp
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
set(HEADERS
    OpUndoItemList.h
    OpUndoItemMuting.h
    OpUndoItemRecorder.h
    OpUndoItemValidator.h
    OpUndoItems.h
    MayaUsdUndoBlock.h
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
