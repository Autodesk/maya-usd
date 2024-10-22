# Attribute Editor Template Design

This folder contains the code to display the attributes of selected USD prims
in the Attribute Editor (from now on called simply AE). It uses the normal Maya
system and vocabulary: UI "templates" containing UI "controls".

## Overall Design

The overall design is that the MayaUSD AE template uses a number of custom
controls that each creates a UI section grouping multiple related attributes.
It uses various helper functions and helper classes to achieve its result.

## AE template

The AE template is a class in `ae_template.py`. It goes through each attribute
of the given USD prim and tries to create a custom control to display it and
its related attributes to be grouped together. It keeps a list of attributes
it has already handled and a list of attribute that should be supressed, which
mean they will not be shown in the UI.

The AE template has two functions to manage supressed attributes: `suppress`
supresses the given attribute. `suppressArrayAttribute` which supresses all
array attribute if the option to display is turned off.

The constructor of the AE template calls `buildUI` to fit each attribute in a
custom control. Afterward it calls the functions `createAppliedSchemasSection`,
`createCustomExtraAttrs` and `createMetadataSection` to create some UI sections
that are always present.

The `buildUI` function goes through each USD schema of the USD prim. If the
schema is one of the "well-known" ones that have a specialized UI section,
then that section gets created. For example, shader, transform and display each
have a specialized UI section.

If the schema is not a special one, then `buildUI` retrieves all attributes of
the schema and calls `createSection` to create the UI section that will contain
those attributes. `createSection` checks if the attribute is not supressed and
calls `addControls` to do the UI creation.

The `addControls` function goes through each custom control and ask each of them
to create the UI for the attributes. Once a custom control accepts to handle the
attribute, it calls `defineCustom` to register the custom control with Maya.
Afterward the attributes are added to the list of already handled attributes.
The AE template can then proceed to the next schema.

## Custom Controls

Each custom control is a class with a creator function. The creator function
checks if this custom control can be used for a given attribute and if so,
creates an instance of this custom control for all the grouped supported
attributes. If the attribute is not related to the custom control, the creator
function returns None.

Each custom control has an `onCreate` and a `onReplace` callbacks. The `onCreate`
callback is called by Maya to create the UI of the custom control and fill
it with data from the attributes. The `onReplace` callback is used to update
the previously created UI with new data.

The list of custom controls are:

- ArrayCustomControl: to display attributes containing arrays of data.
- EnumCustomControl: to display enumerated value as a drop-down menu.
- ConnectionsCustomControl: to display connected attribute and go to the target.
- DisplayCustomDataControl: to display custom data for Outliner Color.
- ImageCustomControl: to display the file paths of an image.
- MaterialCustomControl: to display attributes related to materials.
- MetadataCustomControl: to display the metadata of the prim.

## Observers

The AE template uses observers to refresh itself. These observers are custom
controls without any UI elements. They merely re-use the custom control Maya
callbacks to start and stop observing/listening to changes. They start to
observe in their `onCreate` callback and stop in either their `__del__` method
or in a `uiDelete` `scripJob`.

The observers are:

- UfeAttributesObserver: observes and queues calls to _queueEditorRefresh() when
  an attribute is added or removed and when the `xformOpOrder` attribute or the
  `materialBindings` attribute are modified.

- UfeConnectionChangedObserver: observes and calls to _queueEditorRefresh() when
  an attribute connection has changed.

- NoticeListener: listens and calls the `refresh` function of the given custom
  controls when receving the USD ObjectsChanged notification.

## Helpers

There are various helper functions and classes.

- `AEUITemplate` class: used to push/pop the AE template using the `setUITemplate`
  command with the `-pushTemplate` and `-popTemplate` flags.

- UI refresh functions: `_refreshEditor` calls the `refreshEditorTemplates` MEL
  command, which refreshes the AE. `_queueEditorRefresh` queues a deferred call
  to `_refreshEditor` if there are no refresh queued yet.

- `PrimCustomDataEditRouting` class: automatically creates an edit router for prim
  metadata editing.

- `showEditorForUSDPrim` function: deferred call to the `showEditor` MEL command
  to show the prim at the given USD path. Used by the `ConnectionsCustomControl`.

- `AEShaderLayout` class: sort and order attributes. Used by LookdevX.

