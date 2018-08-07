# Translation - A high-level overview

There are quite a few mechanisms for doing various translation-like things:
1. Import Translators - Static conversion of USD Data to equivalent Maya Data e.g meshes - documented [here](importExport.md)
2. Export Translators - Static conversion of Maya Data to equivalent USD Data e.g meshes - documented [here](importExport.md)
3. Schema Translator Plugins - Plugin equivalents of 3. e.g MayaReference (@todo link) - documented [here](schemaTranslatorPlugins.md)
4. Special Cases e.g Transforms - documented [here](animation.md)

At the moment 4. is the only extensible mechanism which allows you to add plugins without modifying the AL_USDMaya source code...This will change!

1. and 2. are exposed as:
+ Maya commands
+ via the Maya File Translator interface.

3. and 4. are generally invoked when ProxyShapeImport is used, and are run for each schema that is encountered for which a Translator is registered. 

Live Transform connections are created when objects are selected in a ProxyShape, or one of the relevant ProxyShape commands is called.

There is also a way of invoking the importers from 1. above by adding a special attribute to each USDPrim called “importToMaya” (@todo: verify this, add some tests! etc)

As you can see there are a number of systems here with different characteristics.
These need to be unified to simplify the code and provide a consistent set of behaviours and entry points. This is on our todo list!

## Utilities and Helpers
However a translator is exposed or implemented, the code [here](https://animallogic.github.io/AL_USDMaya/group__fileio.html) provides a large number of base classes, utility functions and helpers to allow you to implement import/export functionality. These will probably be re-organised in the future!
