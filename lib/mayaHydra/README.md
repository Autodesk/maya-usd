# Hydra for Maya (Technology Preview)

The _Hydra for Maya_ project creates a Maya plugin that replaces the main Maya viewport with a Hydra viewer. _Hydra for Maya_ is developed and maintained by Autodesk. The project and this documentation are a work-in-progress and under active development. The contents of this repository are fully open source and open to contributions under the [Apache license](../../doc/LICENSE.md)!

Hydra is the rendering API inside [Pixar's USD](http://openusd.org/).

## Motivation

The goal for _Hydra for Maya_ is to introduce Hydra as an open source viewport framework for Maya to extend the viewport render engine through Hydra render delegates. _Hydra for Maya_ uses the previous Maya to Hydra (MtoH) code, which is part of MayaUSD, as a foundation to build on. You can find more details on what changed from MtoH [here](./doc/mayaHydraDetails.md). _Hydra for Maya_ is currently a Technology Preview; we are just laying the foundation and there is still work ahead. As the plugin evolves and the Hydra technology matures, you can expect changes to API and to face limitations.

## What can it do?

With the _Hydra for Maya_ plugin, you can:

- Use Storm as the default render delegate for Maya's viewport.
- Render and interact with most of Maya's node types.
- Use different render delegates using USD/Hydra's plugin system.
- Natively view USD data with USD Extension for Maya through HdSceneIndex.
- Use Hydra side-by-side with Maya's current Viewport 2.0.

## Current Limitations

The _Hydra for Maya_ plugin has the following isses and limitations:.

- The USD stage is only viewable. Interactive transform edits are not working with the v22.11 USD release (Fixed by https://github.com/PixarAnimationStudios/USD/commit/9516b96e90).
- UsdGeomCapsule, UsdGeomCone, UsdGeomCube, UsdGeomCylinder, and UsdGeomSphere are not supported. UsdGeomMesh can be used as an alternative.
- MaterialX is not supported.
- Only direct texture inputs are supported for Maya materials.
- Limited support for Maya shader networks.
- Drawing issues with selection and highlighting.
- Hydra shading differs from Maya's Viewport 2.0.
- Blue Pencil is not supported.
- Screen space effects like depth of field and motion blur are not supported through Storm.
- The following Maya node types are not viewable:
  - Bifrost
  - nParticles
  - Fluid
- Arnold lights are not supported with Storm.
- Animation Ghosting has the wrong shading.

## Detailed Documentation
+ [Building the mayaHydra.mll plugin](./doc/mayaHydraBuild.md)
+ [Technical details of MayaHydra](./doc/mayaHydraDetails.md)
