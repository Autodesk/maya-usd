# Maya to Hydra

Render a Maya scene using a Hydra render delegate.

## What's it good for?

The primary motivation behind this project is to use HdStorm, the
OpenGL renderer bundled with USD, as an alternative to
Viewport 2.0.

Why would you want to use HdStorm instead of "normal" Viewport 2.0?

1. Consistent lighting and shading between Hydra-enabled applications: Maya, Katana, usdview, etc
1. HdStorm is open source: you can add core features as you need them
1. HdStorm is extensible: you can create plugins for custom objects
1. HdStorm is under active development by Pixar

Using Hydra also has beneftis for offline renderers. Any renderer that implements a Hydra render delegate can now have a fully interactive render viewport in Maya, along with support for render proxies via AL_USDMaya. This could be particularly useful for newer projects, like Radeon ProRender (which already has a [render delegate](https://github.com/GPUOpen-LibrariesAndSDKs/RadeonProRenderUSD)) or in-house renderers.

## Usage

Load the included plugin, mtoh, located at:

Windows:
`[Maya-USD-Root]\lib\maya\mtoh.dll`

MacOS:
`[Maya-USD-Root]/lib/maya/mtoh.dylib`

Linux:
`[Maya-USD-Root]/lib/maya/mtoh.so`

Then, using the **Renderer** viewport menu, select **GL (Hydra)** - or, if you have another hydra render delegate installed, select **<Renderer> (Hydra)**

## List of supported Maya nodes

Shapes:
- mesh

Lights:
- areaLight
- pointLight
- spotLight
- directionalLight
- aiSkydomeLight

Shaders:
- UsdPreviewSurface
- lambert
- blinn
- file
