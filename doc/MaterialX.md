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
- Place2dTexture
- The ColorCorrect nodes from LookdevKit
- The clamp utility node
- Inserts explicit type conversion nodes where Maya uses implicit conversion
- Exports connections at the component level

### Partially supported:

- Lambert, Blinn, and Phong shader nodes are exported as MaterialX approximations. This allows roundtripping.

### Not supported:

- Layered and ramp surface shaders
- Most of the Maya shader utility nodes
- MaterialXSurface shader nodes from MaterialX Maya contrib

### To enable:

Two options:
- Install MayaUSD v0.16.0 or later
- Rebuild tip of MayaUSD repo using a MaterialX-enabled build of USD

## Import

We can import MaterialX networks.
- UsdShade networks consisting of MaterialX nodes used for shading in the `mtlx` render context
- `.mtlx` files referenced in a USD stage as converted by the `usdMtlx` plugin in USD
- Anything that was exported from Maya

### Not supported:

- MaterialX networks containing surface shaders other than those exported
- MaterialX networks containing unexpected procedural nodes
- Importing to MaterialXSurface shader nodes

### To enable:

Two options:
- Install MayaUSD v0.16.0
- Rebuild tip of MayaUSD repo using a MaterialX-enabled build of USD

## USD stage support in viewport

We support rendering a MaterialX USD asset directly in the VP2 viewport.

### Supported:

We are using the hdMtlx translation framework, which is also in use for hdStorm and hdPrman, and using the same GLSL code generator as hdStorm, so valid USD MaterialX material graphs should display correctly.

Maya spheres with standard surface shading: 
![alt text](./MayaStandardSurfaceSampler.JPG "Standard surface sampler")

The same spheres exported with MaterialX shading and loaded as a USD stage: ![alt text](./USDMaterialXStandardSurfaceSampler.JPG "USD MaterialX sampler")

- Some of the recently exportable Maya nodes require additional MaterialX nodegraph implementations supplied with MayaUSD (Phong, place2dTexture, LookdevKit::ColorCorrect)
- Tangents will be shader-generated. By default they are arbitrarily oriented along the X derivative in screen space, but if a texcoord stream is used in the shader, the tangents will be aligned with the U direction.
- Textures marked as sRGB will be automatically converted to Maya working color space. Limited to these working spaces: "scene-linear Rec.709-sRGB", "ACEScg", "ACES2065-1", "scene-linear DCI-P3 D65", and "scene-linear Rec.2020".

### Not supported:

- DirectX 11 viewport

### To enable:

Two options:
- Install MayaUSD v0.16.0 on top of Maya 2022.3 or PR132
- Rebuild tip of MayaUSD repo using a MaterialX-enabled build of USD and a supported Maya version

Once the updated plugin is in use, the viewport will automatically select MaterialX shading over UsdPreviewSurface shading if the referenced USD stage contains MaterialX shading networks.

### Building a MaterialX-enabled USD compatible with MayaUSD

Requires patching USD:

For USD 22.05b:
```
git checkout tags/v22.05b
# Fetch updates for MaterialX 1.38.4:
git cherry-pick ac3ca253e643a095e0985264238be28b95109a44
git cherry-pick 034df39a78a5fc4cada49b545ad2a87fd6666e1d
git cherry-pick aaf20de564e29c3e59bcd32b3539add08b7597c6
git cherry-pick 8a1ad41d27e2f1b9595a7404a7f8a89dce5cb5bc
git cherry-pick e6edb7e8fd74f3f0d5cd981450df0c937a809257
```

Tip of USD dev branch is already using MaterialX 1.38.4

Then you build USD using the `build_usd.py` script and the `--materialx` option.
