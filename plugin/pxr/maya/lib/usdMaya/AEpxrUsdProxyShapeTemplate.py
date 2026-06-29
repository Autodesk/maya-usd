#
# Copyright 2026 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from pxr import UsdMaya

from maya import cmds
from maya import mel

import functools


# ========================================================================
# VARIANT SETS
# ========================================================================

def _GetVariantSetInfoFromNode(node, variantSetName):
    '''Returns (override, settable) for the variantSetName on node.'''
    variantAttrName = 'usdVariantSet_%s' % variantSetName
    override = ''
    settable = True
    if cmds.attributeQuery(variantAttrName, node=node, exists=True):
        variantSetPlgVal = cmds.getAttr('%s.%s' % (node, variantAttrName))
        if variantSetPlgVal:
            override = variantSetPlgVal
        settable = cmds.getAttr('%s.%s' % (node, variantAttrName), settable=True)
    return override, settable


def _variantSets_changeCommand(unused, omg, node, variantSetName):
    val = cmds.optionMenuGrp(omg, q=True, value=True)

    AuthorVariantSelectionFromAE(node, variantSetName, val)

    # Update the resolved variant selection label
    resolvedVariant = ''
    usdPrim = UsdMaya.GetPrim(node)
    if usdPrim:
        variantSet = usdPrim.GetVariantSet(variantSetName)
        if variantSet:
            resolvedVariant = variantSet.GetVariantSelection()
    cmds.optionMenuGrp(omg, edit=True, extraLabel=resolvedVariant)


def AuthorVariantSelectionFromAE(node, variantSetName, variantSelection):
    variantAttr = 'usdVariantSet_%s' % variantSetName
    if not cmds.attributeQuery(variantAttr, node=node, exists=True):
        cmds.addAttr(node, ln=variantAttr, dt='string', internalSet=True)
    cmds.setAttr('%s.%s' % (node, variantAttr), variantSelection, type='string')


def variantSets_Replace(nodeAttr, new):
    origParent = cmds.setParent(q=True)

    frameLayoutName = 'AEpxrUsdProxyShapeTemplate_variantSets_Layout'
    if new:
        cmds.frameLayout(frameLayoutName, label='VariantSets', collapse=False)
    else:
        cmds.setParent(frameLayoutName)

    # Remove existing children of layout
    children = cmds.frameLayout(frameLayoutName, q=True, childArray=True)
    if children:
        for child in children:
            cmds.deleteUI(child)

    node = nodeAttr.split('.', 1)[0]
    usdPrim = UsdMaya.GetPrim(node)
    if usdPrim:
        for variantSetName in usdPrim.GetVariantSets().GetNames():
            usdVariantSet = usdPrim.GetVariantSet(variantSetName)
            variantResolved = usdVariantSet.GetVariantSelection()
            variantSetChoices = [''] + usdVariantSet.GetVariantNames()
            variantOverride, variantSettable = _GetVariantSetInfoFromNode(
                node, variantSetName)

            omg = cmds.optionMenuGrp(
                label=variantSetName,
                enable=variantSettable,
                extraLabel=variantResolved)
            for choice in variantSetChoices:
                cmds.menuItem(label=choice)

            try:
                cmds.optionMenuGrp(omg, e=True, value=variantOverride)
            except RuntimeError:
                cmds.warning('Invalid choice %r for %r'
                             % (variantOverride, variantSetName))

            cmds.optionMenuGrp(omg, e=True,
                changeCommand=functools.partial(
                    _variantSets_changeCommand,
                    omg=omg, node=node, variantSetName=variantSetName))

    cmds.setParent(origParent)


def variantSets_Replace_new(nodeAttr):
    variantSets_Replace(nodeAttr, new=True)

def variantSets_Replace_replace(nodeAttr):
    variantSets_Replace(nodeAttr, new=False)


# ========================================================================
# FILE PATH
# ========================================================================

def filePath_Replace(nodeAttr, new):
    if new:
        cmds.setUITemplate('attributeEditorTemplate', pushTemplate=True)
        cmds.rowLayout(numberOfColumns=3)
        cmds.text(label='File Path')
        cmds.textField('pxrUsdProxyShape_usdFilePathField')
        cmds.symbolButton('pxrUsdProxyShape_usdFileBrowserButton',
                          image='navButtonBrowse.xpm')
        cmds.setParent('..')
        cmds.setUITemplate(popTemplate=True)

    def _showFileBrowser(*args):
        filePaths = cmds.fileDialog2(
            caption="Specify USD File",
            fileFilter="USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc)",
            fileMode=1)
        if filePaths:
            cmds.setAttr(nodeAttr, filePaths[0], type='string')
    cmds.button('pxrUsdProxyShape_usdFileBrowserButton', edit=True,
                command=_showFileBrowser)

    cmds.evalDeferred(functools.partial(
        cmds.connectControl, 'pxrUsdProxyShape_usdFilePathField', nodeAttr))


def filePath_Replace_new(nodeAttr):
    filePath_Replace(nodeAttr, new=True)

def filePath_Replace_replace(nodeAttr):
    filePath_Replace(nodeAttr, new=False)


# ========================================================================
# EDITOR TEMPLATE
# ========================================================================

def editorTemplate(nodeName):
    cmds.editorTemplate(beginScrollLayout=True)

    isSubcomponent = UsdMaya.IsSubcomponentProxy(nodeName)

    cmds.editorTemplate(beginLayout='USD', collapse=False)
    cmds.editorTemplate(
        'filePath',
        callCustom=[filePath_Replace_new, filePath_Replace_replace])
    cmds.editorTemplate('primPath', addControl=True)
    cmds.editorTemplate('excludePrimPaths', addControl=True)
    cmds.editorTemplate('variantKey', addControl=True)
    cmds.editorTemplate('complexity', addControl=True)
    cmds.editorTemplate('drawRenderPurpose', addControl=True)
    cmds.editorTemplate('drawProxyPurpose', addControl=True)
    cmds.editorTemplate('drawGuidePurpose', addControl=True)
    cmds.editorTemplate(endLayout=True)

    if isSubcomponent:
        cmds.editorTemplate(beginLayout='Variant Sets', collapse=False)
        cmds.editorTemplate(
            '',
            callCustom=[variantSets_Replace_new, variantSets_Replace_replace])
        cmds.editorTemplate(endLayout=True)

    mel.eval('AEsurfaceShapeTemplate "%s"' % nodeName)
    cmds.editorTemplate(addExtraControls=True)

    cmds.editorTemplate(endScrollLayout=True)


# ========================================================================
# MEL STUBS
# ========================================================================

def addMelFunctionStubs():
    '''Create MEL proc that delegates to Python for the AE template.'''
    mel.eval('''
global proc AEpxrUsdProxyShapeTemplate( string $nodeName )
{
    python("AEpxrUsdProxyShapeTemplate.editorTemplate('"+$nodeName+"')");
}
    ''')
