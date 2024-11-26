from maya import cmds

import ufe
import usdUfe
import mayaUsd.ufe

from collections import defaultdict
from functools import partial

from mayaUSDRegisterStrings import getMayaUsdString


############################################################################
#
# Manage pretty names

_pluginNiceNames = {
    "usdGeom": "Geometry",
    "usdLux": "Lighting",
    "mayaUsd_Schemas": "Maya Reference",
    "usdMedia": "Media",
    "usdRender": "Render",
    "usdRi": "RenderMan",
    "usdShade": "Shading",
    "usdSkel": "Skeleton",
    "usdUI": "UI",
    "usdVol": "Volumes",
    "usdProc": "Procedural",
    "usdPhysics": "Physics",
    "usdArnold": "Arnold",
    # Skip legacy AL schemas
    "AL_USDMayaSchemasTest": "",
    "AL_USDMayaSchemas": "",
}

def _getNicePluginName(pluginName):
    '''
    Convert the plugin name to a nicer name we designed for users.
    Return an empty name for plugin we don't want to show.
    '''
    return _pluginNiceNames.get(pluginName, pluginName)


def _prettifyMenuItem(pluginName, prettyTypeName):
    '''
    Make the schema type name more pretty.
    '''
    # Note: Python str removesuffix and removeprefix were only added in Python 3.9,
    #       so we can't use them because we need to support Python 2.
    if prettyTypeName.endswith('API'):
        prettyTypeName = prettyTypeName[:-3]

    # Note: make sure that the type name is not the same as the plugin name,
    #       otherwise we would end-up with an empty name! This happens for example
    #       for the light schema. One of them is called LightAPI...
    if prettyTypeName != pluginName and prettyTypeName.startswith(pluginName):
        prettyTypeName = prettyTypeName[len(pluginName):]

    prettyTypeName = mayaUsd.ufe.prettifyName(prettyTypeName) 

    return prettyTypeName


############################################################################
#
# Schema and selection queries

def _groupSchemasByPlugins(schemas):
    '''
    Group schemas by the plugin name that contains them and then
    by the schema type name, giving a boolean flag if the schema
    is a multi-apply schema.
    '''
    schemaByPlugins = defaultdict(dict)
    for info in schemas:
        pluginName = _getNicePluginName(info['pluginName'])
        if not pluginName:
            continue
        schemaByPlugins[pluginName][info['schemaTypeName']] = info['isMultiApply']
    return schemaByPlugins


def _getUsdItemsInSelection():
    '''
    Retrieve the UFE items in the selection that are USD prims.
    '''
    ufeSelection = ufe.GlobalSelection.get()
    return [item for item in ufeSelection if item.runTimeId() == mayaUsd.ufe.getUsdRunTimeId()]


def _getUsdPathsInSelection():
    '''
    Retrieve the UFE paths of the USD items in the selection.
    '''
    return [ufe.PathString.string(item.path()) for item in _getUsdItemsInSelection()]


def _askForInstanceName(prettyTypeName):
    '''
    Ask the user for the multi-apply schema instance name.
    '''
    title = getMayaUsdString("kAddSchemaMenuItem")
    message = getMayaUsdString("kAddSchemaInstanceMessage") % prettyTypeName
    cancelLabel = getMayaUsdString("kButtonCancel")
    AddLabel = getMayaUsdString("kButtonAdd")

    result = cmds.promptDialog(
        message=message, title=title,
        button=(AddLabel, cancelLabel),
        defaultButton=AddLabel, cancelButton=cancelLabel)
    if result != AddLabel:
        return ''

    return cmds.promptDialog(query=True, text=True)


############################################################################
#
# Invoke and echo schema commands

def _quoted(text):
    '''Quote text if it is text, taking care to escape quotes.'''
    if type(text) is not str:
        return str(text)
    return '"' + text + '"'


def _invokeSchemaCommand(primUfePaths, **kwargs):
    '''
    Build the schema command and both run and echo it.
    '''
    cmd = 'mayaUsdSchema'
    for arg, value in kwargs.items():
        cmd += ' -' + arg
        if type(value) is not bool:
            cmd +=' ' + _quoted(value)
    if primUfePaths:
        cmd += ' ' + ' '.join([_quoted(path) for path in primUfePaths])
    cmd += ';'
    return cmds.mayaUsdSchema(primUfePaths, **kwargs)


############################################################################
#
# Apply schema

def _applySchemaMenuItemCallback(prettyTypeName, schemaTypeName, isMultiApply, menuItem):
    '''
    Apply the given schema to the prims in the current selection.
    '''
    primPaths = _getUsdPathsInSelection()
    if not primPaths:
        cmds.warning(getMayaUsdString("kCannotApplySchemaWarning") % schemaTypeName)
        return

    if isMultiApply:
        instanceName = _askForInstanceName(prettyTypeName)
        if not instanceName:
            return
        _invokeSchemaCommand(primPaths, schema=schemaTypeName, instanceName=instanceName)
    else:
        _invokeSchemaCommand(primPaths, schema=schemaTypeName)


def _createApplySchemaCommand(prettyTypeName, schemaTypeName, isMultiApply):
    '''
    Create a menuitem callback that will apply the given schema and update its metadata
    to have a nice undo item entry in the Maya Edit menu.
    '''
    f = partial(_applySchemaMenuItemCallback, prettyTypeName, schemaTypeName, isMultiApply)
    # Update the function metadata so that we get a nice entry in the Maya
    # undo menu.
    nonBreakSpace = '\xa0'
    f.__module__ = ('Apply Schema ' + prettyTypeName).replace(' ', nonBreakSpace)
    f.__name__ = ''
    return f


def _createApplySchemaMenuItem(pluginMenu, pluginName, schemaTypeName, isMultiApply):
    '''
    Create a menu item for the schema that will apply it to the selection.
    We create them in a function to avoid problems with variable capture
    in the lambda.
    '''
    prettyTypeName = _prettifyMenuItem(pluginName, schemaTypeName)
    suffix = '...' if isMultiApply else ''
    cmds.menuItem(
        label=prettyTypeName + suffix, parent=pluginMenu,
            command=_createApplySchemaCommand(prettyTypeName, schemaTypeName, isMultiApply))
    

def _createAddSchemasMenuItem(parentMenu, mayaVersion):
    '''
    Create a "Add Schema" menu items with all schemas as sub-items.
    '''
    hasUSDPrimInSelection = bool(len(_getUsdItemsInSelection()))
    addMenu = cmds.menuItem(
        label=getMayaUsdString("kAddSchemaMenuItem"), version=mayaVersion//10000,
        subMenu=True, parent=parentMenu, enable=hasUSDPrimInSelection)

    # Make sure plugin names are presented in a predictable order, sorted by their pretty names.
    schemaByPlugins = _groupSchemasByPlugins(usdUfe.getKnownApplicableSchemas())
    pluginNames = sorted([(mayaUsd.ufe.prettifyName(name), name) for name in schemaByPlugins.keys()])

    for prettyPluginName, pluginName in pluginNames:
        schemas = schemaByPlugins[pluginName]
        pluginMenu = cmds.menuItem(label=prettyPluginName, subMenu=True, parent=addMenu)

        # Make sure the schema names are presented in a predictable order, sorted by their pretty names.
        schemaNames = sorted([(_prettifyMenuItem(pluginName, name), name) for name in schemas.keys()])

        for _, schemaTypeName in schemaNames:
            isMultiApply = schemas[schemaTypeName]
            _createApplySchemaMenuItem(pluginMenu, pluginName, schemaTypeName, isMultiApply)


############################################################################
#
# Remove schema

def _removeSchemaMenuItemCallback(prettyTypeName, schemaTypeName, instanceName, menuItem):
    '''
    Remove the given schema from the prims in the current selection.
    '''
    primPaths = _getUsdPathsInSelection()
    if not primPaths:
        cmds.warning(getMayaUsdString("kCannotRemoveSchemaWarning") % schemaTypeName)
        return

    _invokeSchemaCommand(primPaths, schema=schemaTypeName, instanceName=instanceName, rem=True)


def _createRemoveSchemaCommand(prettyTypeName, schemaTypeName, instanceName):
    '''
    Create a menuitem callback that will remove the given schema and update its metadata
    to have a nice undo item entry in the Maya Edit menu.
    '''
    f = partial(_removeSchemaMenuItemCallback, prettyTypeName, schemaTypeName, instanceName)
    # Update the function metadata so that we get a nice entry in the Maya
    # undo menu.
    nonBreakSpace = '\xa0'
    f.__module__ = ('Remove Schema ' + prettyTypeName).replace(' ', nonBreakSpace)
    f.__name__ = ''
    return f


def _createRemoveSchemaMenuItem(pluginMenu, pluginName, schemaTypeName, instanceName):
    '''
    Create a menu item for the schema that will remove it to the selection.
    We create them in a function to avoid problems with variable capture
    in the lambda.
    '''
    prettyTypeName = _prettifyMenuItem(pluginName, schemaTypeName)
    if instanceName:
        prettyTypeName += ': ' + mayaUsd.ufe.prettifyName(instanceName)
    cmds.menuItem(
        label=prettyTypeName, parent=pluginMenu,
            command=_createRemoveSchemaCommand(prettyTypeName, schemaTypeName, instanceName))
    

def _createRemoveSchemasMenuItem(parentMenu, mayaVersion):
    '''
    Create a "Remove Schema" menu items with all schemas of the given prims.
    '''
    schemaTypeNameAndInstanceNames = _invokeSchemaCommand(_getUsdPathsInSelection(), appliedSchemas=True)

    hasSchemaToRemove = bool(len(schemaTypeNameAndInstanceNames))
    addMenu = cmds.menuItem(
        label=getMayaUsdString("kRemoveSchemaMenuItem"), version=mayaVersion//10000,
        subMenu=True, parent=parentMenu, enable=hasSchemaToRemove)
    
    if not hasSchemaToRemove:
        return
    
    # Multi-apply schemas are reported as `schemaTypeName:instance-name`, so split the names.
    # For single-apply schemas, the split will have a single item, the schema type name.
    schemaTypeNameAndInstanceNames = [sch.split(':') for sch in schemaTypeNameAndInstanceNames]

    # Make sure plugin names are presented in a predictable order, sorted by their pretty names.
    schemaByPlugins = _groupSchemasByPlugins(usdUfe.getKnownApplicableSchemas())
    pluginNames = sorted([(mayaUsd.ufe.prettifyName(name), name) for name in schemaByPlugins.keys()])

    for prettyPluginName, pluginName in pluginNames:
        schemas = schemaByPlugins[pluginName]

        # Only create the plugin menu if it will have items in it.
        hasCreatedMenu = False

        # Make sure the schema names are presented in a predictable order, sorted by their pretty names.
        schemaNames = sorted([(_prettifyMenuItem(pluginName, name), name) for name in schemas.keys()])

        for _, schemaTypeName in schemaNames:
            for schAndInst in schemaTypeNameAndInstanceNames:
                if schemaTypeName != schAndInst[0]:
                    continue

                if not hasCreatedMenu:
                    pluginMenu = cmds.menuItem(label=prettyPluginName, subMenu=True, parent=addMenu)
                    hasCreatedMenu = True

                instanceName = schAndInst[1] if len(schAndInst) > 1 else ''
                _createRemoveSchemaMenuItem(pluginMenu, pluginName, schemaTypeName, instanceName)


############################################################################
#
# Menu creation entry point

def aeAttributesMenuCallback(aeAttributeMenu):
    cmds.menuItem(
        divider=True, dividerLabel=getMayaUsdString("kUniversalSceneDescription"),
        parent=aeAttributeMenu)
    
    mayaVersion = cmds.about(apiVersion=True)

    _createAddSchemasMenuItem(aeAttributeMenu, mayaVersion)
    _createRemoveSchemasMenuItem(aeAttributeMenu, mayaVersion)

