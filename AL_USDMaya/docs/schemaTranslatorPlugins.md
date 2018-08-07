# Schema Translators

The main entry point for mapping data described in USD form into some Maya form is the Schema Translator.

In general we have a mapping betweeen a "Typed" USDSchema and a Schema Translator, and when AL_USDMaya import functionality encounters a schema that it has a translator for, it translates it.

A schema translator can do many things e.g
- it can convert a USD prim hierarchy to one or more Maya DAG Nodes, DG Nodes etc
- set up parameters in the maya scene (e.g start/end frame range)
- connect USD attributes to their corresponding maya attributes and drive the maya thing from USD (e.g we could drive a maya camera from a USD camera)
- copy data from a USD prim to a matching maya prim
- do fairly arbitrary stuff in the maya scene
- As We can also define USD schemas for things that USD doesn't understand natively e.g maya files - we can therefore have a schema for MayaReferences, and add a Schema Translator which import any maya references .
- Translators can also just delegate work to existing pipeline code (e.g written in python) that will wire the appropriate objects up using maya commands etc.

We are hoping that as USD evolves, the number of things that need to be handled as Maya References or other DCC-Native types will reduce - e.g there may be support for constraints, animation etc which would mean our motion could be stored as native USD data 


Internally at Animal Logic, we also have schemas, and matching schema translators for things such as:
- Shot Information (e.g start + end frame, audio data etc)
- Background Plates
- Motion


The translator interface is defined in TranslatorAbstract.h
It supports a number of methods e.g:
- import
- postImport
- tearDown
- update 
..which are documented with in the code for [TranslatorAbstract.h](https://github.com/AnimalLogic/AL_USDMaya/blob/master/lib/AL_USDMaya/AL/usdmaya/fileio/translators/TranslatorAbstract.h)

Note: at the moment there is no export support


## Built-in Schema Translators
+ There is a test translator called TranslatorTestType.h, but no usable non-plugin Schema Translators


##  Schema Translator Plugins
see [translators](../translators/README.md) 


## Custom Schema Translator Plugins
For information on Implementing your own see [translator plugins](https://animallogic.github.io/AL_USDMaya/translator_plugins.html) for an example
@todo: validate this with Rob

## Tech Note: The SchemaNodeRef Database 
This class stores a mapping between each USD Prim Path and Maya Path
