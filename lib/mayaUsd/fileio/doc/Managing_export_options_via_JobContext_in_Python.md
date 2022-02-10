# Managing import and export options via Job Contexts

Complex import and export scenarios often require enabling a specific set of import and export
options. The Job Context protocol enables efficient management of these options.

## Creating a Job Context

To register a job context in C++, use the `REGISTER_IMPORT_JOB_CONTEXT_FCT` and `REGISTER_EXPORT_JOB_CONTEXT_FCT` macros:

```c++
REGISTER_EXPORT_JOB_CONTEXT_FCT(
    MyContext, 
    "UI name for MyContext", // Same for import and export.
    "Tooltip for MyContext when used for export")
{
    // Enabler function code. Returns a VtDictionary of export options
    // to be used when MyContext is chosen for export.
    VtDictionary args;
    args[UsdMayaJobExportArgsTokens->shadingMode]
        = VtValue(std::string("useRegistry"));
    args[UsdMayaJobExportArgsTokens->convertMaterialsTo] = VtValue(
        std::vector<VtValue> { VtValue(std::string("MyContextMaterials")) });
    args[UsdMayaJobExportArgsTokens->apiSchema] = VtValue(std::vector<VtValue> {
         VtValue(std::string("MyContextLightApi")),
         VtValue(std::string("MyContextShadowApi")) });
    args[UsdMayaJobExportArgsTokens->chaser]= VtValue(std::vector<VtValue> { 
        VtValue(std::string("MyContextMeshChaser")),
        VtValue(std::string("MyContextXformChaser")) });
    std::vector<VtValue> chaserArgs;
    chaserArgs.emplace_back(std::vector<VtValue> {
        VtValue(std::string("MyContextMeshChaser")),
        VtValue(std::string("frobiness")),
        VtValue(std::string("42")) });
    chaserArgs.emplace_back(std::vector<VtValue> {
        VtValue(std::string("MyContextXformChaser")),
        VtValue(std::string("precision")),
        VtValue(std::string("5")) });
    args[UsdMayaJobExportArgsTokens->chaserArgs] = VtValue(chaserArgs);
    return args;
}
```

A Python version of this API is also available:

```python
import mayaUsd.lib as mayaUsdLib

def MyContextExportEnabler():
    """Enabler function code. Returns a dictionary of export options
    to be used when MyContext is chosen for export."""
    return {
        "shadingMode" : "useRegistry",
        "convertMaterialsTo" : ["MyContextMaterials"],
        "apiSchema" : ["MyContextLightApi", "MyContextShadowApi"],
        "chaser" : ["MyContextMeshChaser", "MyContextXformChaser"],
        "chaserArgs" : [["MyContextMeshChaser", "frobiness", "42"],
                        ["MyContextXformChaser", "precision", "5"]]
    }

mayaUsdLib.JobContextRegistry.RegisterExportJobContext(
    MyContext, 
    "UI name for MyContext",
    "Tooltip for MyContext when used for export",
    MyContextExportEnabler)
```

## MayaUsd plugin support:

### Exporting via command line:

It is possible to explicitly specify a job context in the export options:

The following Python command:
```python
cmds.mayaUSDExport(
    mergeTransformAndShape=True,
    file="MyScene", 
    jobContext=["MyContext"]
    )
```
Is fully equivalent to:
```python
cmds.mayaUSDExport(
    mergeTransformAndShape=True,
    file="MyScene", 
    shadingMode="useRegistry",
    convertMaterialsTo=["MyContextMaterials"],
    apiSchema=["MyContextLightApi", "MyContextShadowApi"],
    chaser=["MyContextMeshChaser", "MyContextXformChaser"],
    chaserArgs=[["MyContextMeshChaser", "frobiness", "42"],
                ["MyContextXformChaser", "precision", "5"]]
    )
```

### Import and export file dialogs:

When opening the export options for USD, there will be a dropdown menu called "Plug-in Configuration:" which lists all registered job contexts. If a user selects "UI name for MyContext" in the menu, then the export will be done using the options provided by the enabler callback.

The UI will also react to these options by setting and greying out all the UI visible options affected by the job context.

### Interaction with multi-material export:

The MayaUSD plugin supports multi-material export for all registered `useRegistry` shading modes. So the user is still free to export to both "UsdPreviewSurface" and "MyContextMaterials". This is done by merging the contents of the `convertMaterialsTo` array returned by the enabler function with the array provided by the user.

However, if "MyContextMaterials" are only exportable using a legacy UsdMayaShadingModeExporter, then the export option becomes `shadingMode="MyContextMaterials"`, and only these materials will be exported.