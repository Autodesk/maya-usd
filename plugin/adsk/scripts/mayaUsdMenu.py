from maya import cmds

import ufe
import usdUfe
import mayaUsd.ufe

from collections import defaultdict
from functools import partial

from mayaUSDRegisterStrings import getMayaUsdString

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


def _askForInstanceName(prettyTypeName):
    '''
    Ask the user for the multi-apply schema instance name.
    '''
    title = getMayaUsdString("kAddSchemaMenuItem")
    message = getMayaUsdString("kAddSchemaInstanceMesage") % prettyTypeName

    result = cmds.promptDialog(
        message=message, title=title,
        button=('OK', 'Cancel'),
        defaultButton='OK', cancelButton='Cancel')
    if result != 'OK':
        return ''

    return cmds.promptDialog(query=True, text=True)


def _applySchemaMenuItemCallback(prettyTypeName, schemaTypeName, isMultiApply, menuItem):
    '''
    Apply the given schema to the prims in the current selection.
    '''
    primPaths = [ufe.PathString.string(item.path()) for item in _getUsdItemsInSelection()]
    if not primPaths:
        print('Cannot apply schema "%s": no USD prim are currently selected.' % schemaTypeName)
        return

    if isMultiApply:
        instanceName = _askForInstanceName(prettyTypeName)
        if not instanceName:
            return
        cmds.mayaUsdSchema(primPaths, schema=schemaTypeName, instanceName=instanceName)
    else:
        cmds.mayaUsdSchema(primPaths, schema=schemaTypeName)


def _createApplySchemaCommand(prettyTypeName, schemaTypeName, isMultiApply):
    '''
    Create a menuitem callback for teh given argument andwith its metadata updated
    to have a nice undo item entry in the Maya Edit menu.
    '''
    f = partial(_applySchemaMenuItemCallback, prettyTypeName, schemaTypeName, isMultiApply)
    # Update the function metadata so that we get a nice entry in the Maya
    # undo menu.
    nonBreakSpace = '\xa0'
    f.__module__ = ('Apply Schema ' + prettyTypeName).replace(' ', nonBreakSpace)
    f.__name__ = ''
    return f


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


def _createSchemaMenuItem(pluginMenu, pluginName, schemaTypeName, isMultiApply):
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


def aeAttributesMenuCallback(aeAttributeMenu):
    mayaVersion = cmds.about(apiVersion=True)
    cmds.menuItem(
        divider=True, dividerLabel=getMayaUsdString("kUniversalSceneDescription"),
        parent=aeAttributeMenu)

    hasUSDPrimInSelection = bool(len(_getUsdItemsInSelection()))
    addMenu = cmds.menuItem(
        label=getMayaUsdString("kAddSchemaMenuItem"), version=mayaVersion//10000,
        subMenu=True, parent=aeAttributeMenu, enable=hasUSDPrimInSelection)

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
            _createSchemaMenuItem(pluginMenu, pluginName, schemaTypeName, isMultiApply)
