# Maya to Hydra

Render a Maya scene using a Hydra-compatible render delegate.

## What's it good for?

The primary motivation behind this project is to use HdStream -- the
OpenGL renderer bundled with USD -- as an alternative to
Viewport 2.0.

There are several advantages to HdStream over Viewport 2.0:

1. Ability to cast shadows between [AL_USDMaya](www.github.com/AnimalLogc/AL_usdMaya) proxy shapes and real Maya shapes
2. Consistent lighting and shading between Hydra-enabled applications: Maya, Katana, usdview, etc
3. HdStream is under active development by Pixar
2. HdStream is open source: you can add core features as you need them
3. HdStream is extensible: you can create plugins for custom objects


## Status

This project is still very alpha.  We're looking for contributors.

## Building

Targets maya 2018 and the latest USD dev branch.
