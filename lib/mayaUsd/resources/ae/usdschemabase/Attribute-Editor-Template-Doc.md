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

The constructor of the AE template calls a series of `find`-ing functions to
fit each attribute in a custom control or other special built-in sections.
Afterward it calls `orderSections` to choose the order in which the sections
will be added to the AE template. It then calls `createSchemasSections` to
create the section UI.

Each `find` function goes through the USD schemas of the USD prim. Some of
the `find` functinare looking for specific schemas. Other are looking for
applied schema or class schemas. A few are not looking at schemas at all
but are grabbing specific kind of attributes. Here are the `find` functions:

- findAppliedSchemas: find the attributes belonging to applied schemas.
- findClassSchemasL find the attributes belonging to the prim class schemas.
- findSpecialSections: find the shader, transforms and display attributes.

The `orderSections` function has a list of section it wishes to place first and
a list of sections it wishes to place last. It orders all the sections that were
found according to those lists. All remaining sections that are not forced to be
in a particular order are placed in between.

The `createSchemasSections` function takes the ordered sections and call the
correct function for each. There is a generic `createSection` to create the UI
for normal sections. There are a few special `create` functions for the special
sections, like shader, material, transforms, metadata, etc.

The creator function called `createCustomCallbackSection` is special. it allows
sections to be created in a user-supplied callback. See below for more info.

The `createSection` function checks if the attribute is not supressed and calls
`addControls` to do the UI creation. The `addControls` function goes through each
custom control creator and ask each of them to create the UI for the attributes.
Once a custom control accepts to handle the attribute, it calls `defineCustom`
to register the custom control with Maya. Afterward the attributes are added to
the list of already handled attributes.


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

## Callbacks

There is a special custom callback section `createCustomCallbackSection()`
called during `createSchemasSections()` in the AE template that gives users
the opportunity to add layout sections to the AE template. For example the
Arnold plugin which has its own USD prims. Using this callback section a plugin
can organize plugin specific attributes of a prim into AE layout section(s).

To use this callback section a plugin needs to create a callback function and
then register for the AE template UI callback:

```python
# AE template UI callback function.
import ufe
import maya.internal.common.ufe_ae.template as ufeAeTemplate
from ufe_ae.usd.nodes.usdschemabase.ae_template import AETemplate as mayaUsd_AETemplate

# Callback function which receives two params as input: callback context and
# callback data (empty).
def onBuildAETemplateCallback(context, data):
    # In the callback context you can retrieve the ufe path string.
    ufe_path_string = context.get('ufe_path_string')
    ufePath = ufe.PathString.path(ufe_path_string)
    ufeItem = ufe.Hierarchy.createItem(ufePath)

    # The callback data is empty.

    # Then you can call the special static method above to get the MayaUsd
    # AE template class object. Using that object you can then create
    # layout sections and add controls.
    #
    # Any controls added should be done using mayaUsdAETemplate.addControls()
    # which takes care of checking the AE state (show array attributes, nice
    # name, etc). It also adds the added attributes to the addedAttrs list.
    # This will keep the attributes from also appearing in the "Extra
    # Attributes" section.
    #
    # If you don't want attributes shown in the AE, you should call
    # mayaUsdAETemplate.suppress().
    #
    # You can also inject a function into the attribute nice naming.
    # See [Attribute Nice Naming callback] section below for more info.
    #
    mayaUsdAETemplate = mayaUsd_AETemplate.getAETemplateForCustomCallback()
    if mayaUsdAETemplate:
        # Create a new section and add attributes to it.
        with ufeAeTemplate.Layout(self.mayaUsdAETemplate, 'My Section', collapse=True):
            self.mayaUsdAETemplate.addControls(['param1', 'param2'])

# Register your callback so it will be called during the MayaUsd AE
# template UI building code. This would probably done during plugin loading.
# The first param string 'onBuildAETemplate' is the callback operation and the
# second is the name of your callback function.
import mayaUsd.lib as mayaUsdLib
from usdUfe import registerUICallback
mayaUsdLib.registerUICallback('onBuildAETemplate', onBuildAETemplateCallback)

# During your plugin unload you should unregister the callback (and any attribute
# nice naming function you also added).
mayaUsdLib.unregisterUICallback('onBuildAETemplate', onBuildAETemplateCallback)
```

## Attribute Nice Naming callback

A client can add a function to be called when asked for an attribute nice name.
That function will be called from `getNiceAttributeName()` giving the client
the ability to provide a nice name for the attribute.

The callback function will receive as input two params:
- ufe attribute
- attribute name

And should return the adjusted name or `None` if no adjustment was made.

See `attributeCustomControl.getNiceAttributeName()` for more info.