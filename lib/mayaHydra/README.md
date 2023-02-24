# Hydra for Maya (Technology Preview)

_Hydra for Maya_ introduces Hydra as a viewport framework for Maya and it allows extending the viewport render engine through Hydra render delegates. Unlike _MtoH_ it utilizes a new API for efficient translation of Maya or USD data.
To ensure Autodesk builds the right foundation, this API is subject to change. Because of this _Hydra for Maya_ is currently a Technology Preview and not feature complete. In addition there are also current limitations in the Hydra framework which are under consideration.

## What it can do?
- Use Storm as the default render delegate for Maya's viewport
- Render and interact with most of Maya's node types
- Use different render delegates using USD/Hydra's plugin system
- Natively view USD data with USD Extension for Maya through SceneIndex rather than SceneDelegates
- Use it side-by-side with Maya's current Viewport 2.0

## Maya DAG node scene index registration

DAG node scene index registration enables the use of custom scene indices for drawing of custom Maya shapes via Hydra prims. Internally, we are using this technology to enable maya-usd with the new Hydra viewport with no dependencies on maya-usd from maya-hydra and vice-versa.

The registration code is called on the first, render override frame, when Hydra resources are initialized (MtohRenderOverride::_InitHydraResources). Upon first initialization, or when a node is added, a scene index is created. In the case of MayaUsd the nodes in question are MayaUsdProxyShapeBase. The scene index defined by the MayaUsdProxyShapeMayaNodeSceneIndex simply wraps UsdImagingSceneIndex which knows how to draw Usd data.


## Current Limitations
This is a list of known issues and limitations.

- USD stage viewable only. Interactive transform edits are not working with the chosen v22.11 USD release (Fixed by https://github.com/PixarAnimationStudios/USD/commit/9516b96e90).
- UsdGeomCapsule, UsdGeomCone, UsdGeomCube, UsdGeomCylinder, and UsdGeomSphere are not supported. Use the supported UsdGeomMesh to create such geometries.
- MaterialX is not supported
- Basic support for Maya materials (only direct texture inputs)
- Hypershade nodes are not supported except texture inputs
- Drawing issues with selection and highlighting
- Shading differences with Maya's Viewport 2.0
- Blue Pencil is not supported
- Screen space effects like depth of field and motion blur are not supported through Storm
- Following Maya node types are not viewable:
    - Bifrost
    - nParticles
    - Fluid
- Arnold lights are not supported with Storm
- Animation Ghosting has wrong shading


## Detailed Documentation
+ [Building the mayaHydra.mll plugin](./doc/mayaHydraBuild.md)
+ [Differences between MayaHydra and Mtoh (Luma Pictures)](./doc/mayaHydraVsMtoh.md)
