# Maya Reference Edit Router Documentation

The Maya USD plugin supports hosting a Maya reference inside the USD scene.
This Maya reference can be edited as Maya data and then cached as USD prims.
The caching process supports customization of its behaviour, even a full
replacement of its logic through an `edit router`.

This document outlines the API of this `edit router` and what the default
implementation does.


## Edit Router Generalities

The edit router is a callback that receives information in a USD `VtDictionary`
and fills another `VtDictionary` with information about what work it did.

The interface to declare and register an `edit router` is found in the file
`lib\mayaUsd\utils\editRouter.h`. It declares the `EditRouter` class itself
and functions to register and retrieve an `edit router`. The `edit router`
is registered and retrieved by name.


## Maya Reference Edit Router

The Maya Reference `edit router` is named "mayaReferencePush". The default
implementation found in the file `lib\mayaUsd\utils\editRouter.cpp` as the
function named `cacheMayaReference` does the following:

- Extract all necessary inputs from the input `VtDictionary`.
- Validate the path of the Maya Reference that was edited.
- Validate the destination layer and destination prim.
- Determine the file format of the USD cache.
- Export the edited Maya data into a USD layer, the USD cache.
- Copy the transform of the root of the Maya data into the Maya Reference.
- Refer to the USD cache either as a reference or payload, optionally
  in a variant.
- Fill the output `VtDictionary` with information about the layer and prim.

We will go into more details about these steps in the following sections.

### Maya Reference Edit Router Inputs

The input information received by the Maya Reference `edit router` are:

- "prim" (std::string): the `SdfPath` of the USD Maya Reference that was edited. 
- "defaultUSDFormat" (std::string): the USD file format used to create the cache.
- "rn_layer" (std::string): the file path to the cache. A layer will be created there.
- "rn_primName" (std::string): the name of the root prim of the cache.
- "rn_listEditType" (std::string): how the reference is added: "Append" or "Prepend".
- "rn_payloadOrReference" (std::string): type of ref: "Reference" or "Payload".
- "rn_defineInVariant" (int): if the cache is in a variant. 0 or 1.
- "rn_variantSetName" (std::string): name of the variant set if in a variant.
- "rn_variantName" (std::string): name of the variant in the set, if in a variant.
- "src_stage" (UsdStageRefPtr): the source `UsdStage` to copy the transform.
- "src_layer" (SdfLayerRefPtr): the source `SdfLayer` to copy the transform.
- "src_path" (SdfPath): the source `SdfPath` to copy the transform.
- "dst_stage" (UsdStageRefPtr): the destination `UsdStage` to copy the transform.
- "dst_layer" (SdfLayerRefPtr): the destination `SdfLayer` to copy the transform.
- "dst_path" (SdfPath): the destination `SdfPath` to copy the transform.

The last six inputs correspond to the parameters received by the Maya Reference
updater `pushCopySpecs` function. That is the function that is calling the
`edit router`.

### Validation and Export

The Maya Reference default `edit router` validates various inputs to verify
that the Maya Reference exists and was edited. It then exports the Maya scene
nodes of the Maya Reference file to a USD layer file. That layer file will be
used as the USD cache representing the contents of the Maya Reference.

### Copy the Transform

To support that the user might have moved the root of the Maya Reference when
editing as Maya data, the transform of this root is copied to the USD Maya
Reference. This preserve the position of the data.

### Create the Cache Prim

The USD cache must be referenced in the USD stage. The `edit router` creates
a reference or payload, possibly in a variant, inside a USD prim. Remember
that the path to that prim was given in "rn_primName".

### Fill the Output VtDictionary

The output `VtDictionary` is filled with the following information:

- "layer" (std::string): the identifier of the cache layer, will be equal to "rn_layer".
- "save_layer" (std::string): "yes" if the layer should be saved. (Calling `Save` on it.)
- "path" (std::string): the `SdfPath` to the prim that contains the cache.
