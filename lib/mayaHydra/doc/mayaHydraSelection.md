# Selection in MayaHydra

## Summary:
The new custom scene index API encapsulates interaction with the customer data model using the following entry points: `InterpretRprimPath` and `HandleSelectionChanged`. This is done by letting plugins populate the methods into function pointer references provided as a data source at the moment the `_AppendSceneIndex` factory method is called by the customer-defined scene index plugin. Please note that this is a temporary measure while we are deciding on the proper code-sharing structure.

## Interpreting Picking Result

For each pick hit, we rely on the `InterpretRprimPath` function to determine which Maya node or (UFE data item) is selected. If the resulting `Ufe::Path`'s runtime is the Maya runtime, then MayaHydra simply adds the corresponding Maya DAG node to the `MSelectionList`. If the resulting `Ufe::Path` originates from any other runtime, then MayaHydra simply adds the `Ufe::Path` to the `Ufe::NamedSelection`. The two different code paths must be maintained, as opposed to adding Maya `Ufe::Path` to the Ufe Selection. At the moment, `Ufe::SceneItem` created from a Maya `Ufe::Path` are ignored.

## Examples

As examples for the custom scene index API please look at how MayaUsd defines the `MayaUsdProxyShapeSceneIndex` and implements the `InterpretRprimPath` and `HandleSelectionChanged`. `InterpretRprimPath` returns the `Ufe::Path` complete with its USD runtime, which corresponds to the given rprim `SdfPath`. `HandleSelectionChanged` is left empty for the moment. However, with the given `Ufe::Path`, it is possible recursively to determine which rprim will be affected by the selection. In turn, the `DirtiedPrimEntries` will be defined in such a way to activate the prims' wireframe on surface display when the items are selected.

The `MayaHydraExample` is a simpler use case for registration and selection using a custom scene index.
