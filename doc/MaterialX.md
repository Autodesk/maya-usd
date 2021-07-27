# MaterialX

Support for MaterialX in MayaUSD

## Support level

Support for MaterialX in Maya USD is based on USD support provided by the usdMtlx and hdMtlx modules.

This means we use a UsdShade translation of the MaterialX graph as transport mechanism.

## Export

We currently support exporting to MaterialX-compatible UsdShade networks:

- Standard surface shader nodes
- USD Preview Surface shader nodes
- File texture nodes
    - Connections to color components and alpha/luminance
    - Normal maps
    - Custom color spaces

### Not supported:

- Lambert, Blinn, and Phong shader nodes
- MaterialXSurface shader nodes from MaterialX Maya contrib

### To enable:

- Rebuild the MayaUsd plugin for Maya PR126 using tip of the dev branch or any commit that includes [PR 1478](https://github.com/Autodesk/maya-usd/pull/1478)
- Look for a new "MaterialX shading" option in the `Materials` dropdown of the export options

## Import

We can import MaterialX networks.
- UsdShade networks consisting of MaterialX nodes used for shading in the `mtlx` render context
- `.mtlx` files referenced in a USD stage as converted by the `usdMtlx` plugin in USD

### Not supported:

- MaterialX networks containing surface shaders other than standard surface or preview surface
- MaterialX networks containing unexpected procedural nodes
- Importing to MaterialXSurface shader nodes

### To enable:

- Rebuild the MayaUsd plugin for Maya PR126 using tip of the dev branch or any commit that includes [PR 1478](https://github.com/Autodesk/maya-usd/pull/1478)
- The import code will discover MaterialX shading networks and attempt to import them automatically

## USD stage support in viewport

We support rendering a MaterialX USD asset directly in the VP2 viewport.

### Supported:

We are using the hdMtlx translation framework, which is also in use for hdStorm and hdPrman, and using the same GLSL code generator as hdStorm, so valid USD MaterialX material graphs should display correctly.

Maya spheres with standard surface shading: 
![alt text](./MayaStandardSurfaceSampler.JPG "Standard surface sampler")

The same spheres exported with MaterialX shading and loaded as a USD stage: ![alt text](./USDMaterialXStandardSurfaceSampler.JPG "USD MaterialX sampler")

### Not supported:

- DirectX 11 viewport
- [Issue 1523](https://github.com/PixarAnimationStudios/USD/issues/1523): Color spaces
- [Issue 1538](https://github.com/PixarAnimationStudios/USD/issues/1538): MaterialX networks containing surface shaders other than standard surface or preview surface

### To enable:

This one is more complex and requires knowledge of how to build USD.

**Requires Maya PR126**

1. Build MaterialX using the [Autodesk fork of MaterialX](https://github.com/autodesk-forks/MaterialX)
    - Requires version [v1.38.1_adsk](https://github.com/autodesk-forks/MaterialX/releases/tag/v1.38.1_adsk) or later
    - Requires at minimum MATERIALX_BUILD_CONTRIB, MATERIALX_BUILD_GEN_OGSXML, and MATERIALX_BUILD_SHARED_LIBS options to be enabled
    - We only require the OGS XML shadergen part (and USD requires the GLSL shadergen), so you can turn off all complex options like Viewer, OIIO, OSL, or Python support
2. Install that freshly built MaterialX in the `install` location where you intend to build USD next
3. Build a recent USD from the tip of the dev branch into the `install` location that has the updated MaterialX from step 1 (assuming you will use the build_usd.py script)
    - There is an API change in the Autodesk branch of MaterialX that requires updating `pxr/imaging/hdSt/materialXFilter.cpp` at line 71 to remove second parameter on the `isTransparentSurface()` function call
4. Rebuild MayaUSD plugin from the tip of the dev branch or any commit that includes both [PR 1478](https://github.com/Autodesk/maya-usd/pull/1478) and [PR 1433](https://github.com/Autodesk/maya-usd/pull/1433)
    - Requires enabling the CMAKE_WANT_MATERIALX_BUILD option
    - MaterialX_DIR should point to the one built in step 1
    - PXR_USD_LOCATION should point to one built in step 3

Once the updated plugin is in use, the viewport will automatically select MaterialX shading over UsdPreviewSurface shading if the referenced USD stage contains MaterialX shading networks.