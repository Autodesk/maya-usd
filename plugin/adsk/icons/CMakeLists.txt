# ---------------------------------------------------------------------------------------------
# install
# ---------------------------------------------------------------------------------------------

set(PLUGIN_ICONS
    out_mayaUsdProxyShape
    USD_generic
    USD_stage
)
foreach(ICON_BASE ${PLUGIN_ICONS})
    # The _100.png files need to be installed without the _100. This is the
    # base icon name that is used. Maya will automatically choose the _150/_200
    # image if neeeded.
    install(FILES "${ICON_BASE}_100.png"
        DESTINATION "${INSTALL_DIR_SUFFIX}/icons"
        RENAME "${ICON_BASE}.png"
    )
    install(FILES "${ICON_BASE}_150.png" "${ICON_BASE}_200.png"
        DESTINATION "${INSTALL_DIR_SUFFIX}/icons"
    )
endforeach()

install(FILES "mayaUsdProxyShape.svg"
    DESTINATION "${INSTALL_DIR_SUFFIX}/icons"
)

# Special case - install files for proxy shape base
#                (same icon as proxy shape).
set(MAYAUSDPROXYSHAPE "mayaUsdProxyShape")
install(FILES "out_${MAYAUSDPROXYSHAPE}_100.png"
    DESTINATION "${INSTALL_DIR_SUFFIX}/icons"
    RENAME "out_${MAYAUSDPROXYSHAPE}Base.png"
)
install(FILES "out_${MAYAUSDPROXYSHAPE}_150.png"
    DESTINATION "${INSTALL_DIR_SUFFIX}/icons"
    RENAME "out_${MAYAUSDPROXYSHAPE}Base_150.png"
)
install(FILES "out_${MAYAUSDPROXYSHAPE}_200.png"
    DESTINATION "${INSTALL_DIR_SUFFIX}/icons"
    RENAME "out_${MAYAUSDPROXYSHAPE}Base_200.png"
)
install(FILES "${MAYAUSDPROXYSHAPE}.svg"
    DESTINATION "${INSTALL_DIR_SUFFIX}/icons"
    RENAME "${MAYAUSDPROXYSHAPE}Base.svg"
)
