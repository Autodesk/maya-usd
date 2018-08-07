# Translator plugin system

## Intro

When loading a usd file in maya (either importing it or via a proxy shape, *see usdMaya documentation*),  usdMaya loops over each prim. If a prim is not handled internally and it is a schema type prim, usdMaya will look for any registered translators plugin bound to this prim type. The plugin has a chance to do some initialization before import occurs. Then import is done in two steps, ```Import``` is called for every prim then ```PostImport``` is called for the same prims to give them a chance to establish connection with maya nodes created during first import step.

*The export process still has to be defined*

## Quick start
 * add a new class to AL_USDMayaTranslators
 * use AL_USDMAYA_DEFINE_TRANSLATOR macro to bind the plugin to the translated usd schema type
```
AL_USDMAYA_DEFINE_TRANSLATOR(TranslatorTestPlugin, TranslatorTestType)
```
 * implement
  * Initialize
  * Import
  * PostImport
  * Export
  * TearDown

Some context will be available to plugins. Currently a stage data and a proxy shape pointers will be available if the translation is being done via a proxy shape.
```c++
UsdStageRefPtr stage = TranslatorStageDataUtils::GetStage(GetContext());
```
A mapping storage is also available
```c++
GetContext()->AddPrim(usdPrim, mayaObject);
```
```c++
MObject object;
if(GetContext()->GetMObject(prim, object)) {
  // lets do it
}
```
A TearDown helper also exists that allows you to automatically remove any additional maya nodes you created within your Import function, e.g.

Within your Import:
```c++
GetContext()->AddPrim(usdPrim, mayaObject);

MFnDependencyNode fn;
MObject newObj = fn.create("myCustomNodeType");
GetContext()->insertItem(usdPrim.GetPath(), newObj);
```
Within TearDown:

```c++
GetContext()->removerItems(primPath);
```

*The context model is very likely to change in the near future though*

## Implementations details

### Plugin system

The plugin system relies heavily on usd plug and registry mechanisms. And more precisely on the fact that you can declare generic ```TfType``` types and derived types which can be queried later and associate a factory to them.
The C++ class ```TranslatorBase``` will be the base ```TfType``` which will be queried for derived types (the actual translators). It also embedds the ```Manufacture``` method which is called by usdMaya to build the appropriate translator.
```TranslatorFactory``` is a templated class that will build the translator object during the manufacturing.
By being a derived ```TranslatorBase```, a ```TfType``` can easily be looked for and will be associated with a factory object that will instanciate the actual translator object.

So a translator plugin must meet the following properties:
 
 * derive from the C++ class ```TranslatorBase```
 * register in usd a ```TfType```
  * derived from ```TranslatorBase```
  * declaring a specialized version of ```TranslatorFactory```
```c++
  TfType::Define<plugClass, TfType::Bases<TranslatorBase>>()
  .SetFactory<TranslatorFactory<plugClass>>();
```
 * implement a static ```New(TranslatorContextPtr context)``` method

### Plugin context

*to be defined*

## Todos

 * python plugin support
 * roundtripping test
 * add a maya plugin metadata entry in plugInfo.json to be able to declare any needed maya plugins
 * extract ```Manufacture``` from ```TranslatorBase```

## Known limitations

 * *Export* is not currently used
 * context mapping storage only maps a prim path to a maya object
 * translators are created for every schema prim
