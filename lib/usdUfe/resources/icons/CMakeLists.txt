# ---------------------------------------------------------------------------------------------
# install
# ---------------------------------------------------------------------------------------------

# TreeView icons
set(TREEVIEW_ICONS
    BlendShape
    Camera
    Capsule
    Class
    CompArcBadge
    CompArcBadgeV
    Cone
    Cube
    Cylinder
    Def
    GeomSubset
    LightFilter
    LightPortal
    Material
    Mesh
    NodeGraph
    NurbsPatch
    PhysicsJoint
    Plane
    PluginLight
    PointInstancer
    Points
    Procedural
    Render
    Scope
    Shader
    SkelAnimation
    Skeleton
    SkelRoot
    Sphere
    UI
    UsdGeomCurves
    UsdGeomXformable
    UsdLuxBoundableLightBase
    UsdLuxNonboundableLightBase
    UsdTyped
    Volume
)
foreach(ICON_BASE ${TREEVIEW_ICONS})
    # The _100.png files need to be installed without the _100. This is the
    # base icon name that is used. A DCC (such as Maya) will automatically
    # choose the _150/_200 image if neeeded.
    install(FILES "out_USD_${ICON_BASE}_100.png"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/icons"
        RENAME "out_USD_${ICON_BASE}.png"
    )
    install(FILES "out_USD_${ICON_BASE}_150.png" "out_USD_${ICON_BASE}_200.png"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/icons"
    )
endforeach()
