# Example Import and Export Plugins in C++

This folder contains an example of a MayaUSD import and export plugin written in C++.
It demonstrates:
- How to hook into the export process to force some options on or off.
- How to present a UI to the user to set options specific to your export plugin.
- How to save your options in a persistent Maya option variable.
- How to register this export plugin with MayaUSD.

## Contents of the example

The example export plugin is made up of the following files:

- [README.md](README.md): this documentation
- [mayaUsdPlugInfo.json](mayaUsdPlugInfo.json): tells MayaUSD how to load the plugin.
  Only needed when using the "stand-alone" loading method described below.
- [resources/plugInfo.json](plugInfo.json): tells USD how to load the plugin.
- [CMakeLists.txt](CMakeLists.txt): the CMake file containing the build instructions
  to compile the plugin.
- [plugin.cpp](plugin.cpp): the C++ entry-points of the Maya plugin.
- [exampleExportPlugin.cpp](exampleExportPlugin.cpp): the C++ code for the export plugin.
- [exampleImportPlugin.cpp](exampleImportPlugin.cpp): the C++ code for the import plugin.
- [helpers.cpp](helpers.cpp): helper functions to write the import and export plugins.
- [helpersJSON.cpp](helpers.cpp): helper functions to handle JSON.
- [helpersUI.cpp](helpers.cpp): helper functions to create user interfaces.
- [helpers.h](helpers.h): declarations of the helper functions.

## Contents of each file

### mayaUsdPlugInfo.json

This JSON file contains the relative path to the folder that contains the USD
JSON file. Here is the file in its entirety:

```JSON
{
    "MayaUsdIncludes": [
        {
            "PlugPath":"exampleImportExportPlugin"
        }
    ]
}
```

### plugInfo.json

This files describes the plugin to the plugin system of USD. The description is
done through the following JSON properties:

- `Name`: the name of the plugin file, either a DLL or Python module.
  This name must not contain the file extension.
- `Type`: the type of plugin, either `library` for a DLL or `python` for Python.
- `Root`: the root of all other file paths, this root being relative to where
  the JSON file itself is located. We specify `..` here as the DLL lives
  in the parent folder.
- `LibraryPath`: the path to the DLL relative to `Root`. Can also be an absolute path.
- `Info/UsdMaya/JobContextPlugin`: the entry tells USD the type of the plugin.
  Here `JobContextPlugin` means it is an export plugin.

Here is the contents of example JSON file. Note that the library path is generated
by the CMake build system:

```JSON
{
    "Plugins": [
        {
            "Name": "exampleImportExportPlugin",
            "Type": "library",
            "Root": "..",
            "LibraryPath": "exampleImportExportPlugin.mll",
            "Info": {
                "UsdMaya": {
                    "JobContextPlugin": {
                    }
                }
            }
        }
    ]
}
```

### ExampleImportExportPlugin.cpp

This is the Python code for the export plugin. We won't cover everything that
is in there, only the overall structure of the code.

#### Registering the plugin

The import and export plugins are registered using C++ macros. These macros
automatically register the plugin with MayaUSD. They are:

```C++
REGISTER_EXPORT_JOB_CONTEXT_FCT()       // Register the export plugin
REGISTER_EXPORT_JOB_CONTEXT_UI_FCT()    // Register the export UI (optional)

REGISTER_IMPORT_JOB_CONTEXT_FCT()       // Register the import plugin
REGISTER_IMPORT_JOB_CONTEXT_UI_FCT()    // Register the import UI (optional)
```
The first callback is the options callback. It is called to retrieve the dictionary
of options that the import or export plugin wants to control. Those options will
no longer be editable by the user in the main MayaUSD export UI. Either those
values are now hard coded by the export plugin or you provide your own new UI
in your UI callback. You can also return options that are specific to your plugin
here.

The second callback is the UI callback. It is called when the user clicks the
`options` button associated with your plugin in the main MayaUSD export UI.
In this callback, you must present a modal dialog to edit the plugin options.
It receives your job context name, the name of the parent UI that triggered
this callback and the current plugin settings. The callback must return the
new settings.

Since both callbacks are expected to return the same settings value for options
controlled by the import or export plugin, the design expects the export plugin
to manage these options itself and save them itself.

Note that the built-in options used by the export, meaning those not controlled
by the export plugin, are saved by MayaUSD. But, given that the options callback
does *not* receive the current options as input, it is necessary that the export
plugin keeps a copy of the current values of each options it wants to control.


#### Managing the plugin settings

The plugins must save and load their particular settings. We chose to convert
a dictionary of values into JSON text and save that text in a Maya option var.

We recommend *not* controlling existing settings. We support it in case your
plugin has special needs that require forcing the setting to a specific value.
For example to ensure that the value is within a special range or that it is
always on or always off.

The code is in these functions:

```C++
getDefaultExportSettings()  // Get defaults for the plugin settings.
loadExportSettings()        // Load the plugin settings, with defaults if needed.
saveExportSettings()        // Save the plugin settings in a Maya opion var.

// Idem for import
getDefaultImportSettings()
loadImportSettings()
saveImportSettings()
```

#### Managing the plugin UI

The plugin UI is built and shown by using the Qt framework and Maya SDK.

```C++
showExportUI()      // Create the UI dialog and calls the next two functions
                    // and save the new settings
fillExportUI()      // Fill the dialog with UI element and their values
queryExportUI()     // Extract the values from the UI elements
```

## How to register the plugin with MayaUSD

The plugin needs to be registered with MayaUSD. There are two possible ways
to do this.

### As a stand-alone Maya plugin

One way is as a stand-alone plugin. To make the plugin discoverable, you
need to tell Maya about your plugin. That is done by setting an environment
variable to the location of the plugin `mll` (DLL) file:

- Set the `MAYA_PLUG_IN_PATH` to the absolute file path where the `mll` file
  containing your plugin is located.
- In Maya, go into the `preferences` `Plug-ins` dialog and load your plugin.
- There is also a standard way to register such plugin with Maya with a `mod`
  file. We won't cover that here as it is already explained in details in the
  Maya documentation.

### As a USD plugin

The other way to auto-load the export plugin is to use the USD plugin system:

- Set the environment variable `MAYA_PXR_PLUGINPATH_NAME` to point to the folder
  containing the `mayaUsdPlugInfo.json` file related to your plugin, containing
  a `MayaUsdIncludes/PlugPath` property that points to the folder containing the
  `plugInfo.json` file.
- Write a `plugInfo.json` file in that folder, containing:
    - A `Name` property containing the name of the plugin.
    - A `Type` property set to `library`.
    - A `Root` property set to `..` so that it finds the DLL.
    - A `LibraryPath` property set to the file path to the DLL relative to `Root`.
    - A `Plugins/Info/UsdMaya` property set to `JobContextPlugin` so that it
      is recognized as a MayaUSD import or export plugin.
- Have the C++ code in the DLL named above in the `LibraryPath` property.
