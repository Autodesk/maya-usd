# Maya to Hydra

Render a Maya scene using a Hydra render delegate.

## What's it good for?

This project allows you to use Hydra (the pluggable USD render ecosystem)
and Storm (the OpenGL renderer for Hydra, bundled with USD), as an
alternative to Viewport 2.0.

Using Hydra has big benefits for offline renderers: any renderer that
implements a Hydra render delegate can now have an interactive render viewport
in Maya, along with support for render proxies.

As an example, when paired with
[arnold-usd](https://github.com/Autodesk/arnold-usd) (Arnold's USD plugin +
render delegate), it provides an Arnold render of the viewport, where both maya
objects and USD objects (through proxies) can be modified interactively.

This could also be particularly useful for newer renderers, like Radeon
ProRender (which already has a
[render delegate](https://github.com/GPUOpen-LibrariesAndSDKs/RadeonProRenderUSD)),
or in-house renderers, as an easier means of implementing an interactive
render viewport.

What about HdStorm, Hydra's OpenGL renderer? Why would you want to use HdStorm
instead of "normal" Viewport 2.0, given that there are other methods for displaying
USD proxies in Viewport 2.0? Some potential reasons include: 

- Using HdStorm gives lighting and shading between Hydra-enabled applications:
  Maya, Katana, usdview, etc
- HdStorm is open source: you can add core features as you need them
- HdStorm is extensible: you can create plugins for custom objects, which then allows
  them to be displayed not just in Maya, but any Hydra-enabled application

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
- nurbsCurve

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
- phong
- file
