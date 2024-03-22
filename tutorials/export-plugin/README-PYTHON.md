# Python Export Plugin

This documents gives detailed explanations about how the example export plugin
is written. It explains how to modify it to write your own.

## Overall structure

The code can be split into separate pieces:
- Registering the export plugin with MayaUSD
- The MayaUSD callbacks.
- The UI to edit options.
- Saving and loading the options.

## Registering

The export plugin must be registered with MayaUSD. This is done in the `register`
function:
    - It calls `mayaUsd.lib.JobContextRegistry.RegisterExportJobContext` to
      register the export plugin and its options callback.
    - It call `mayaUsd.lib.JobContextRegistry.SetExportOptionsUI` to register
      the UI callback.

The plugin is always referenced by it job context name. That name must be unique.
We declare that name at the top of the file and use it repeatedly.

## Callbacks

The first callback is the options callback. It is called to retrieve the dictionary
of options that the export plugin wants to control. Those options will no longer
be editable by the user in the main MayaUSD export UI. Either those values are now
hard coded by the export plugin or you provide your own new UI in your UI callback.

You can also return options that are specific to your plugin here. These options
will be passed in the arguments dictionary during export.

In the example, this callback is called `exportEnablerFn`. It returns a global
dictionary containing the forced options. That dictionary variable is called
`exampleSettings` and is declared at the top of the file.

The second callback is the UI callback. It is called when the user clicks the
`options` button associated with your plugin in the main MayaUSD export UI.
In this callback, you should present a modal dialog to edit the options the
user can edit. It receives your job context name, the name of the parent UI
that triggered this callback and the current options settings. The callback
must return the new settings.

Since both the options callback and the UI callback are expected to return the
same settings value for options controlled by the export plugin, the design
expects the export plugin to manage these options itself and save them itself.

Note that the total options used by the export, including those not controlled
by the export plugin, are saved by MayaUSD. But, given that the options callback
does *not* receive the current options as input, it is necessary that the export
plugin keeps a copy of the current values of each options it wants to control.

## UI

In the example, the UI callbacks calls `showUI`, `fillUI` and `queryUI` to create
the dialog, fill it with data and retrieve data when confirmed by the user. These
functions are standard `Qt` UI functions, creating a message dialog with text
boxes, etc in standard `Qt` fashion.

## Saving

The example saves the options it controls as JSON serialized in text in a Maya
option var, which are saved within the Maya preferences. Both `loadSettings` and
`saveSettings` use the option var. The `loadSettings` function is called when
the plugin is registered. The `saveSettings` is called whenever the user confirms
changes in the UI.
