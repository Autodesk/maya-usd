# Example Import and Export Plugins in Python

This folder contains an example of a MayaUSD export plugin written in Python. It demonstrates:
- How to hook into the export process to force some options on or off.
- How to present a UI to the user to set options specific to your export plugin.
- How to save your options in a persistent Maya option variable.
- How to register this export plugin with MayaUSD.

There is also an example import plugin. Everything that applies to the export
plugin has an equivalent. often identical, in the import plugin. That includes:
- Hook into the import process to force some options on or off.
- Callback to present a UI to the user to set options specific to your export plugin.
- Register this import plugin with MayaUSD.

## Contents of the example

The example export plugin is made up of the following files:

- [ExampleExportPlugin.py](ExampleExportPlugin.py): the Python code for the export plugin.
- [README.md](README.md): this documentation
- [README-PYTHON.md](README-PYTHON.md): documentation about the Python code.
- [mayaUsdPlugInfo.json](mayaUsdPlugInfo.json): tells MayaUSD how to load the plugin.
  Only needed when using the "stand-alone" loading method described below.
- [resources/plugInfo.json](resources/plugInfo.json): tells USD how to load the plugin.

The example import plugin is in this additional file:
- [ExampleImportPlugin.py](ExampleImportPlugin.py): the Python code for the import plugin.

## Contents of each file

### mayaUsdPlugInfo.json

This JSON file contains the relative path to the folder that contains the USD
JSON file. Here is the file in its entirety:

```JSON
{
    "MayaUsdIncludes": [
        {
            "PlugPath":"resources"
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
  the JSON file itself is located. We specify `..` here as the Python code lives
  in the parent folder.
- `Info/UsdMaya/JobContextPlugin`: the entry tells USD the type of the plugin.
  Here `JobContextPlugin` means it is an export plugin.

Here is the contents of example JSON file:

```JSON
{
    "Plugins": [
        {
            "Name": "ExampleExportPlugin",
            "Type": "python",
            "Root": "..",
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

### ExampleExportPlugin.py

This is the Python code for the export plugin. For detailed explanations, refer
to [README-PYTHON.md](README-PYTHON.md).

### ExampleImportPlugin.py

This is the Python code for the import plugin. For detailed explanations, refer
to [README-PYTHON.md](README-PYTHON.md).

## How to register the plugin with MayaUSD

The plugin needs to be registered with MayaUSD. There are three possible ways
to do this.

### Extra Maya plugin

One way is to write an extra Maya plugin and have that plugin load and setup
the export plugin. We won't go into the details of how to write a Maya plugin
here as it is already documented by Maya. This is also the more complicated way,
unless you were to write your plugin in C++, in which case the code for both
plugins can be combined into the Maya plugin. In that case, you could follow
the instructions on how to write an export plugin from
[these other instructions](../../lib/mayaUsd/fileio/doc/Managing_export_options_via_JobContext_in_Python.md).

### As a stand-alone plugin

Another way is as a stand-alone plugin. To make the plugin discoverable, you
need to tell both USD and MayaUSD about your plugin, in a chain that ultimately
lead to your plugin. The chain of reference is as follow:

- Set the environment variable `MAYA_PXR_PLUGINPATH_NAME` to point to the folder
  containing all files related to your plugin.
- Set the environment variable `PYTHONPATH` to point to the same folder.
- Write a `mayaUsdPlugInfo.json` file in your plugin folder that has a
  `MayaUsdIncludes/PlugPath` property that points to a `resources` sub-folder.
- Write a `plugInfo.json` file in the `resources` sub-folder that has:
    - A `Name` property containing the Python module name of the plugin.
    - A `Type` property set to `python`.
    - A `Root` property set to `..` so that it finds the Python module.
    - A `Plugins/Info/UsdMaya` property set to `JobContextPlugin` so that it
      is recognized as a MayaUSD export plugin.
- Have the python code in the module named above in the `Name` property.
  Usually, this will be a file named the same way with a `.py` extension.

### As a extra plugin inside the MayaUSD installation

The final way to auto-load the export plugin is to install it within the MayaUSD
plugin itself. It is similar to the stand-alone method above, but a little bit
simpler:

- Create a folder in the `lib/usd` folder of the installed MayaUSD plugin.
- Set the environment variable `PYTHONPATH` to point to this new folder.
- Create a `resources` sub-folder within that new folder.
- Write a `resources/plugInfo.json` JSON file that has the same content
  as explained in the previous section. (Name, Type)
- Have the python code in the module named above in the `Name` property.
  Usually, this will be a file named the same way with a `.py` extension.
