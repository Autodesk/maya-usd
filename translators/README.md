# Schema Translator Plugins

We currently supply two example plugins which we use in production

## Camera
This translates the built-in USD "Camera" schema.
Supports a minimal but useful set of camera-related parameters, see Camera.h (@todo: add doxygen reference) 


## Frame Range
This translates the ALFrameRange prim, and set up the time range and current time in Maya.
If the time range is not authored in the prim, the startTimeCode and endTimeCode metadata of the usdStage are used if authored.
If nothing was authored, the prim has no effect.


## Maya Reference
This has an accompanying schema called ALMayaReference, defined in [schemas](../schemas/AL/usd/schemas/maya/schema.usda.in)

The ALMayaReference schema allow you to define a Maya Reference in USD. The translator plugin will create the corresponding reference node in maya, and set it's attributes.
Currently supports:
+ mayaReference - the filepath or assetId
+ mayaNamespace - the maya namespace that all of the reference content will be prefixed with

Maya Reference is probably our most complex Translator, and maya references can:

+ be swapped in and out and by variant switching, version change or other events
+ be updated - e.g if we switch a variant and detect that the replacement Prim is also a maya reference, we can update the reference rather than tearing down and re-initialising.
+ be set as inactive after import




  

