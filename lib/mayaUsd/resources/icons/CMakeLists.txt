# ---------------------------------------------------------------------------------------------
# install
# ---------------------------------------------------------------------------------------------

# Maya Outliner icons (most come from UsdUfe)
set(OUTLINER_ICONS
    MayaReference
)
foreach(ICON_BASE ${OUTLINER_ICONS})
    # The _100.png files need to be installed without the _100. This is the
    # base icon name that is used. Maya will automatically choose the _150/_200
    # image if neeeded.
    install(FILES "out_USD_${ICON_BASE}_100.png"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/icons"
        RENAME "out_USD_${ICON_BASE}.png"
    )
    install(FILES "out_USD_${ICON_BASE}_150.png" "out_USD_${ICON_BASE}_200.png"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/icons"
    )
endforeach()

set(LIB_ICONS
    saveOption1
    saveOption2
    saveOption3
    discard_edits
    edit_as_Maya
    merge_to_USD
    cache_to_USD
    orphaned_node_badge
)
foreach(ICON_BASE ${LIB_ICONS})
    # The _100.png files need to be installed without the _100. This is the
    # base icon name that is used. Maya will automatically choose the _150/_200
    # image if neeeded.
    install(FILES "${ICON_BASE}_100.png"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/icons"
        RENAME "${ICON_BASE}.png"
    )
    install(FILES "${ICON_BASE}_150.png" "${ICON_BASE}_200.png"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/icons"
    )
endforeach()
