## Migration Guide

Regarding moving from AL_USDMaya to Autodesk’s USD for Maya Plugin, please find the migration guide [here](MIGRATION_GUIDE.md).

## What is AL_USDMaya?
Represent Maya data in Maya, and USD data in USD -  for example, we use AL_USDMaya to allow native Maya entities such as complex maya referenced rigs (and other assets not easily represented in USD) to be embedded in USD scenes and imported into Maya in their native form. The plugin maintains a "live" connection between the USD and Maya scene, and can respond to various events, keeping the Maya and USD scenes in sync. This affords a dynamic user experience which allows artists to swap in and out different representations of objects in their scene (e.g Rigs for Geometry Caches, different levels of detail etc). Additionally, heavyweight scene elements such as sets/crowds can be represented in OpenGL in the Maya viewport, and manipulated either with USD or Maya tools and UI components.


## Motivation
*Why did we need an alternative USD Maya Bridge to the USDMaya plugin that comes with USD?*

Pixar's [USDMaya plugin](https://graphics.pixar.com/usd/docs/Maya-USD-Plugins.html) was originally designed for quite a specific (Set Dressing) workflow using Maya's Scene Assembly Feature. Maya is our most important DCC app, and while, like in the Pixar plugin, we try and avoid representing most of our data (particularly large and complex things) in the Maya native data model, we do use it as the hub of our Scene Building and artist workflow - our Layout Artists, Animators, Set Dressers and Lighters work mostly in Maya, so it made sense to try and implement something that was more tailored to the way we work at Animal Logic. We played with the idea of modifying/forking the existing Pixar USDMaya, but in the end we diverged so much that it didn't make sense. We do have a plan to merge these two at some point and get the best of both worlds. There is a partial feature comparison [here](docs/pxrcomparison.md)

## Contact
We have a google group for discussion and technical assistance: https://groups.google.com/forum/#!forum/al_usdmaya-discussion. For CLAs or to contact us directly, use email  **usdmaya@al.com.au**.

## Videos
We have some videos explaining some of the workflows we support. We are hoping to add to this as we go:
+ [Animation & Layout](https://youtu.be/RluuvOAXvnk)
+ [Modelling & Surfacing](https://youtu.be/DaxLk6pHijw)


## Detailed Documentation

+ [Building it](docs/build.md)
+ [FAQ](docs/faq.md)
+ [Python](docs/python.md)
+ [Contributing](docs/contributing.md)
+ [Translation](docs/translation.md)
+ [Basic Usage](docs/basicUsage.md)
+ [Maya developer tools and utilities](docs/developer.md)
+ [Proxy Shape](docs/proxyShape.md)
+ [Selection](docs/selection.md)
+ [Schema Translator Plugins](docs/schemaTranslatorPlugins.md)
+ [Events](docs/events.md)
+ [Layers](docs/layers.md)
+ [Animation](docs/animation.md)
+ [Import Export](docs/importExport.md)
+ [Asset Resolution and versioning](docs/assetresolution.md)
+ [Workflows](docs/workflows.md)
+ [Example Plugins](https://github.com/AnimalLogic/StudioExample/README.md)
+ [Testing](docs/testing.md)
+ [Comparison with Pixar USDMaya plugin](docs/pxrcomparison.md)
+ [Extra Data Translator Plugin](docs/extraDataTranslatorPlugins.md)

## Developer Documentation

[Source code documentation](https://animallogic.github.io/AL_USDMaya/)

## How is the repository layed out?

The important parts:

| Location  | Description |
| ------------- | --------------- |
| lib           | The core AL_USDMaya Library  |
| plugin        | maya plugin and test plugin| 
| samples       | mixed bag of test inputs |
| schemas       | Schemas - Maya specific schemas for which we have translators |
| translators   | Reference Schema Translator Plugins | 
| tutorials   | a few tutorials|
| utils | For now just contains the event System (AL_EventSystem) | 
| mayautils | a library of code which depends on Maya only  (AL_MayaUtils) |
| usdmayautils | a library of code which depends on USD and Maya  (AL_USDMayaUtils) |

## Libraries 

| Package | Library  | Folder Hierarchy | Namespaces | Major Dependencies
| ------------- | ------------- | ------------- | ------------- |  ------------- | 
| AL_EventSystem | AL_EventSystem.so  | utils/AL/event  | AL::event |  | 
| AL_MayaUtils | AL_MayaUtils.so  | mayautils/AL/maya/utils  | AL::maya:utils |  Maya
| AL_USDMayaUtils | AL_USDMayaUtils.so  | usdmaya/AL/usdmaya/utils  | AL::usdmaya::utils | USD, Maya


### The core AL_USDMaya library

Best to have a look [here](https://animallogic.github.io/AL_USDMaya/modules.html) but a quick summary here:

| Location  | Description |
| ------------------------------------ | ------------------------------------- |
| **lib/AL_USDMaya/AL/usdmaya/cmds** | Maya MpxCommands  | 
| **lib/AL_USDMaya/AL/usdmaya/fileio** |File Import/Export code including some general purpose USD<->Maya interchange utilities e.g [DGNodeTranslator](lib/AL_USDMaya/AL/usdmaya/fileio/translators/DgNodeTranslator.h) [DAGNodeTranslator](lib/AL_USDMaya/AL/usdmaya/fileio/translators/DagNodeTranslator.h) | 
| **lib/AL_USDMaya/AL/usdmaya/nodes** | Maya MpxNodes such as the proxy Shape  | 





