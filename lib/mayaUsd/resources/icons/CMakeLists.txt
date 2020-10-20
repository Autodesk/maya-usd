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
        mayaReference
        Mesh
        NurbsPatch
        PointInstancer
        Points
        Scope
        SkelAnimation
        Skeleton
        SkelRoot
        Sphere
        UsdGeomCurves
        UsdGeomXformable
        UsdLuxLight
        UsdTyped
        Volume
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
endif()
