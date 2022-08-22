# ---------------------------------------------------------------------------------------------
# install
# ---------------------------------------------------------------------------------------------
if (CMAKE_UFE_V2_FEATURES_AVAILABLE)
    # Maya Outliner icons
    set(OUTLINER_ICONS
        BlendShape
        Camera
        Capsule
        CompArcBadge
        CompArcBadgeV
        Cone
        Cube
        Cylinder
        Def
        GeomSubset
        LightFilter
        LightPortal
        MayaReference
        Mesh
        NurbsPatch
        PluginLight
        PointInstancer
        Points
        Scope
        SkelAnimation
        Skeleton
        SkelRoot
        Sphere
        UsdGeomCurves
        UsdGeomXformable
        UsdTyped
        Volume
        Material
        Shader
    )
    if (PXR_VERSION GREATER_EQUAL 2111)
        list(APPEND OUTLINER_ICONS
            UsdLuxBoundableLightBase
            UsdLuxNonboundableLightBase
        )
    else()
        # In 21.11 UsdLuxLight was removed and replaced with UsdLuxBoundableLightBase.
        # So for the older versions we install the icon with the old name instead.
        install(FILES "out_USD_UsdLuxBoundableLightBase_100.png"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/icons"
            RENAME "out_USD_UsdLuxLight.png")
        install(FILES "out_USD_UsdLuxBoundableLightBase_150.png"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/icons"
            RENAME "out_USD_UsdLuxLight_150.png")
        install(FILES "out_USD_UsdLuxBoundableLightBase_200.png"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/icons"
            RENAME "out_USD_UsdLuxLight_200.png")
    endif()
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
endif()

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
