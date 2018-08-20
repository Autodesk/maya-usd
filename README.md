# Maya to Hydra

Render a Maya scene using a Hydra render delegate.

## What's it good for?

The primary motivation behind this project is to use HdStream, the
OpenGL renderer bundled with USD, as an alternative to
Viewport 2.0.

There are several advantages to HdStream over Viewport 2.0:

1. Ability to cast shadows between [AL_USDMaya](www.github.com/AnimalLogc/AL_usdMaya) proxy shapes and real Maya shapes
1. Consistent lighting and shading between Hydra-enabled applications: Maya, Katana, usdview, etc
1. HdStream is under active development by Pixar
1. HdStream is open source: you can add core features as you need them
1. HdStream is extensible: you can create plugins for custom objects

Using Hydra also has beneftis for offline renderers. Any renderer that implements a Hydra render delegate can now how a fully interactive render viewport in Maya, along with support for render proxies via AL_USDMaya. This could be particularly useful for newer projects, like Radeon ProRender or in-house renderers.

## Status

This project is still alpha.

## Building

Configure the build using cmake and run the build via your favourite build system.

The plugin targets Maya 2018, Linux and the latest USD dev branch, which is built including UsdMaya and UsdImaging. USD_ROOT can be passed to cmake to set the location to the compiled USD libraries.

Requirements:

| Package | Version |
| --- | --- |
| GCC | 4.8.5 |
| USD | 0.8.6 |
| Maya | 2018 |
| Boost | 1.61 |
| TBB | 4.4 |

To enable limited shadow support when using the HdSt render delegate, merge in https://github.com/PixarAnimationStudios/USD/pull/541 to your USD source and add `-DLUMA_USD_BUILD=ON` when running cmake.

## Contributing

The codebase (including the coding style) is highly volatile at this moment, breaking changes can be arrive at any time. When preparing a pull request make sure you run `clang-format` (version 6) before submitting.
