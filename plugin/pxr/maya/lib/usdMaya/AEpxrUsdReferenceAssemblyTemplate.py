#
# Copyright 2016 Pixar
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
from contextlib import contextmanager


# ========================================================================
# CONTEXT MANAGERS
# ========================================================================

@contextmanager
def EditorTemplateBeginScrollLayout():
    '''Add beginLayout/endLayout commands for the context block

    Example:
        with EditorTemplateBeginScrollLayout():
            with EditorTemplateBeginLayout('MyLayout', collapse=True)
                cmds.editorTemplate(addControl='filePath')
    '''
    cmds.editorTemplate(beginScrollLayout=True)
    yield()
    cmds.editorTemplate(endScrollLayout=True)


@contextmanager
def EditorTemplateBeginLayout(name, collapse=True):
    '''Add beginLayout/endLayout commands for the context block

    Example:
        with EditorTemplateBeginLayout('MyLayout', collapse=True):
            cmds.editorTemplate(addControl='filePath')
    '''
    cmds.editorTemplate(beginLayout=name, collapse=collapse)
    yield()
    cmds.editorTemplate(endLayout=True)


@contextmanager
def SetUITemplatePushTemplate():
    '''Add setUITemplate push/pop commands for the context block

    Example:
        with SetUITemplatePushTemplate():
            ...
    '''
    cmds.setUITemplate('attributeEditorTemplate', pushTemplate=True)
    yield()
    cmds.setUITemplate(popTemplate=True)


@contextmanager
def RowLayout(*args, **kwargs):
    cmds.rowLayout(*args, **kwargs)
    yield()
    cmds.setParent('..')

# ========================================================================

def variantSets_changeCommmand(unused, omg, node, variantSetName, authorVarSelFn):
    val = cmds.optionMenuGrp(omg, q=True, value=True)

    authorVarSelFn(node, variantSetName, val)

    # Add the resolved variant selection as a UI label
    resolvedVariant = ''
    usdPrim = UsdMaya.GetPrim(node)
    if usdPrim:
        variantSet = usdPrim.GetVariantSet(variantSetName)
        if variantSet:
            resolvedVariant = variantSet.GetVariantSelection()
    cmds.optionMenuGrp(omg, edit=True, extraLabel=resolvedVariant)

# For maya 2022 and above
def variantSets_Replace_new(nodeAttr, new=True):
    variantSets_Replace(nodeAttr, new=True)

def variantSets_Replace_replace(nodeAttr, new=True):
    variantSets_Replace(nodeAttr, new=False)

def variantSets_Replace(nodeAttr, new):
    # Store the original parent and restore it below
    origParent = cmds.setParent(q=True)

    frameLayoutName = 'AEpxrUsdReferenceAssemblyTemplate_variantSets_Layout'
    if new:
        cmds.frameLayout(frameLayoutName, label='VariantSets', collapse=False)
    else:
        cmds.setParent(frameLayoutName)

    # Remove existing children of layout
    children = cmds.frameLayout(frameLayoutName, q=True, childArray=True)
    if children:
        for child in children:
            cmds.deleteUI(child)

    global _setupVariantSetsFn
    if _setupVariantSetsFn:
        _setupVariantSetsFn(nodeAttr)

    # Restore the original parent
    cmds.setParent(origParent)


def DefaultSetupVariantSetsInAE(nodeAttr):
    SetupRegisteredVariantSetsInAE(nodeAttr, DefaultSetupVariantSetInAE)


def DefaultSetupVariantSetInAE(node, usdVariantSet):
    SetupVariantSetInAE(node, usdVariantSet, AuthorVariantSelectionFromAE)


_regVarSetNames = None
def _GetRegisteredVariantSetNames():
    global _regVarSetNames
    if _regVarSetNames is None:
        from pxr import UsdUtils
        _regVarSetNames = [regVarSet.name
                for regVarSet in UsdUtils.GetRegisteredVariantSets()]
    return _regVarSetNames


def _IsRegisteredVariantSet(node, usdVariantSet):
    '''
    Returns True if there are no registered variantSets OR there are registered
    variantSets and usdVariantSet is one of them.
    '''
    regVarSetNames = _GetRegisteredVariantSetNames()
    if not regVarSetNames:
        # no registered variant sets.  just accept all of them.
        return True

    return usdVariantSet.GetName() in regVarSetNames


def SetupRegisteredVariantSetsInAE(nodeAttr, variantSetSetupFn):
    '''
    Sets up the attribute editor to show all registered variantSets in the attribute editor.

    For each registered variant, it will call variantSetSetupFn.
    '''
    _SetupVariantSetsInAE(nodeAttr, _IsRegisteredVariantSet, variantSetSetupFn)


def _SetupVariantSetsInAE(nodeAttr, variantSetFilterFn, variantSetSetupFn):
    node = nodeAttr.split('.', 1)[0]

    usdPrim = UsdMaya.GetPrim(node)
    if not usdPrim:
        return

    for variantSetName in usdPrim.GetVariantSets().GetNames():
        usdVariantSet = usdPrim.GetVariantSet(variantSetName)
        if not variantSetFilterFn(node, usdVariantSet):
            continue
        variantSetSetupFn(node, usdVariantSet)


def _GetVariantSetInfoFromNode(node, variantSetName):
    '''
    Returns (override, settable) for the variantSetName on node.
    '''
    variantAttrName = 'usdVariantSet_%s' % variantSetName
    override = ''
    settable = True
    if cmds.attributeQuery(variantAttrName, node=node, exists=True):
        variantSetPlgVal = cmds.getAttr('%s.%s' % (node, variantAttrName))
        if variantSetPlgVal:
            override = variantSetPlgVal
        settable = cmds.getAttr('%s.%s' % (node, variantAttrName), settable=True)
    return override, settable


def SetupVariantSetInAE(node, usdVariantSet, authorVarSelFn):
    variantSetName = usdVariantSet.GetName()
    variantResolved = usdVariantSet.GetVariantSelection()
    # we prepend '' to the choices so that an empty override doesn't end up
    # picking the first one which can be misleading.
    variantSetChoices = [''] + usdVariantSet.GetVariantNames()
    variantOverride, variantSettable = _GetVariantSetInfoFromNode(node, variantSetName)

    omg = cmds.optionMenuGrp(
        label=variantSetName,
        enable=variantSettable,
        extraLabel=variantResolved)
    for choice in variantSetChoices:
        cmds.menuItem(label=choice)

    try:
        cmds.optionMenuGrp(omg, e=True, value=variantOverride)
    except RuntimeError:
        cmds.warning('Invalid choice %r for %r' % (variantOverride, variantSetName))

    cmds.optionMenuGrp(omg, e=True, 
        changeCommand=functools.partial(
            variantSets_changeCommmand, 
            omg=omg, node=node, variantSetName=variantSetName,
            authorVarSelFn=authorVarSelFn))


def AuthorVariantSelectionFromAE(node, variantSetName, variantSelection):
    variantAttr = 'usdVariantSet_%s' % variantSetName
    if not cmds.attributeQuery(variantAttr, node=node, exists=True):
        cmds.addAttr(node, ln=variantAttr, dt='string', internalSet=True)
    cmds.setAttr('%s.%s' % (node,variantAttr), variantSelection, type='string')


_setupVariantSetsFn = DefaultSetupVariantSetsInAE
def RegisterSetupVariantSetsFunction(setupVariantSetsFn):
    '''
    Sets the function that is used to populate the attribute editor for Usd
    Prim's variantSets.

    To reset this, you can pass in `DefaultSetupVariantSetsInAE`.

    For custom implementations, see
    - SetupRegisteredVariantSetsInAE
    - SetupVariantSetInAE
    - AuthorVariantSelectionFromAE
    '''
    global _setupVariantSetsFn
    _setupVariantSetsFn = setupVariantSetsFn


# ====================================================================

# For maya 2022 and above
def filePath_Replace_new(nodeAttr):
    filePath_Replace(nodeAttr, new=True)

def filePath_Replace_replace(nodeAttr):
    filePath_Replace(nodeAttr, new=False)

def filePath_Replace(nodeAttr, new):
    if new:
        with SetUITemplatePushTemplate():
            with RowLayout(numberOfColumns=3):
                cmds.text(label='File Path')
                cmds.textField('usdFilePathField')
                cmds.symbolButton('usdFileBrowserButton', image='navButtonBrowse.xpm')

    def tmpShowUsdFilePathBrowser(*args):
        filePaths = cmds.fileDialog2(
            caption="Specify USD File",
            fileFilter="USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc)",
            fileMode=1)
        if filePaths:
            cmds.setAttr(nodeAttr, filePaths[0], type='string')
    cmds.button('usdFileBrowserButton', edit=True, command=tmpShowUsdFilePathBrowser)

    cmds.evalDeferred(functools.partial(cmds.connectControl, 'usdFilePathField', nodeAttr))

# ====================================================================

def editorTemplate(nodeName):
    with EditorTemplateBeginScrollLayout():
        mel.eval('AEtransformMain "%s"' % nodeName)
        
        with EditorTemplateBeginLayout('Usd', collapse=False):
            # Maya 2022 changed the syntax of the callCustom attribute
            if int(cmds.about(version=True)) < 2022:
                cmds.editorTemplate(
                    'AEpxrUsdReferenceAssemblyTemplate_filePath_New',
                    'AEpxrUsdReferenceAssemblyTemplate_filePath_Replace',
                    'filePath',
                    callCustom=True)
            else:
                cmds.editorTemplate(
                    'filePath',
                    callCustom=[filePath_Replace_new, filePath_Replace_replace])
            #cmds.editorTemplate('filePath', addControl=True)

            cmds.editorTemplate('primPath', addControl=True)
            cmds.editorTemplate('excludePrimPaths', addControl=True)
            cmds.editorTemplate('time', addControl=True)
            cmds.editorTemplate('complexity', addControl=True)
            # Note: Specifying python functions directly here does not seem to work.  
            # It looks like callCustom expects MEL functions.
            # Maya 2022 changed the syntax of the callCustom attribute
            if int(cmds.about(version=True)) < 2022:
                cmds.editorTemplate(
                    'AEpxrUsdReferenceAssemblyTemplate_variantSets_New',
                    'AEpxrUsdReferenceAssemblyTemplate_variantSets_Replace',
                    '',
                    callCustom=True)
            else:
                cmds.editorTemplate(
                    '',
                    callCustom=[variantSets_Replace_new, variantSets_Replace_replace])

        mel.eval('AEtransformNoScroll "%s"' % nodeName)
        cmds.editorTemplate(addExtraControls=True)
        
        # suppresses attributes
        
        cmds.editorTemplate(suppress='kind')
        cmds.editorTemplate(suppress='initialRep')
        cmds.editorTemplate(suppress='inStageData')

        cmds.editorTemplate(suppress='assemblyEdits')
        cmds.editorTemplate(suppress='blackBox')
        cmds.editorTemplate(suppress='rmbCommand')
        cmds.editorTemplate(suppress='templateName')
        cmds.editorTemplate(suppress='templatePath')
        cmds.editorTemplate(suppress='viewName')
        cmds.editorTemplate(suppress='iconName')
        cmds.editorTemplate(suppress='viewMode')
        cmds.editorTemplate(suppress='templateVersion')
        cmds.editorTemplate(suppress='uiTreatment')
        cmds.editorTemplate(suppress='customTreatment')
        cmds.editorTemplate(suppress='creator')
        cmds.editorTemplate(suppress='creationDate')
        cmds.editorTemplate(suppress='containerType')
        cmds.editorTemplate(suppress='publishedNode')
        cmds.editorTemplate(suppress='publishedNodeInfo')
        cmds.editorTemplate(suppress='publishedNodeType')


def addMelFunctionStubs():
    '''Add the MEL function stubs needed to call these python functions
    '''
    mel.eval('''
global proc AEpxrUsdReferenceAssemblyTemplate_filePath_New( string $nodeAttr ) 
{
    python("AEpxrUsdReferenceAssemblyTemplate.filePath_Replace('"+$nodeAttr+"', new=True)");
}

global proc AEpxrUsdReferenceAssemblyTemplate_filePath_Replace( string $nodeAttr )
{
    python("AEpxrUsdReferenceAssemblyTemplate.filePath_Replace('"+$nodeAttr+"', new=False)");
}

global proc AEpxrUsdReferenceAssemblyTemplate_variantSets_New( string $nodeAttr ) 
{
    python("AEpxrUsdReferenceAssemblyTemplate.variantSets_Replace('"+$nodeAttr+"', new=True)");
}

global proc AEpxrUsdReferenceAssemblyTemplate_variantSets_Replace( string $nodeAttr )
{
    python("AEpxrUsdReferenceAssemblyTemplate.variantSets_Replace('"+$nodeAttr+"', new=False)");
}

global proc AEpxrUsdReferenceAssemblyTemplate( string $nodeName )
{
    python("AEpxrUsdReferenceAssemblyTemplate.editorTemplate('"+$nodeName+"')");
}
    ''')
