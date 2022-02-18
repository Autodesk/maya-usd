#
# Copyright 2022 Autodesk
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

import maya.cmds as cmds
import maya.mel as mel

import mayaUsd
from mayaUsd.lib import cacheToUsd

from mayaUsdLibRegisterStrings import getMayaUsdLibString
import mayaUsdMayaReferenceUtils as mayaRefUtils

from pxr import Sdf, Tf

import re

# These names should not be localized as Usd only accepts [a-z,A-Z] as valid characters.
kDefaultCacheVariantName = 'Cache'
kDefaultCachePrimName = 'Cache1'

# Cache options in string format, for MEL mayaUsdTranslatorExport() consumption.
_cacheExportOptions = None

# The options string that we pass to mayaUsdTranslatorExport.
kTranslatorExportOptions = 'all;!output-parentscope;!animation-data'

# Dag path corresponding to pulled prim.  This is a Maya transform node that is
# not in the Maya reference itself, but is its parent.
_mayaRefDagPath = None

# Pulled Maya reference prim.
_pulledMayaRefPrim = None

def compositionArcChanged(selectedItem):
    pass

def listEditChanged(selectedItem):
    pass

def variantSetNameChanged(selectedItem):
    # Update the Variant Name option menu from the input Variant Set.
    mayaRefPrimParent = _pulledMayaRefPrim.GetParent()
    cmds.optionMenu('variantNameMenu', edit=True, deleteAllItems=True)

    variantNames = mayaRefPrimParent.GetVariantSet(selectedItem).GetVariantNames()
    cmds.menuItem(getMayaUsdLibString('kMayaRefCreateNew'),
                  parent='variantNameMenu')
    for vName in variantNames:
        cmds.menuItem(label=vName, parent='variantNameMenu')
    # Select 'Create New' by default and set the text field to default value.
    cmds.optionMenu('variantNameMenu', edit=True, select=1)
    cmds.textField('variantNameText', edit=True, visible=True, text=kDefaultCacheVariantName)

def variantNameChanged(selectedItem):
    # If the "Create New" item is selected, we show the text input
    # to the right of the optionMenu. For existing variant names
    # the field is hidden.
    createNew = getMayaUsdLibString('kMayaRefCreateNew')
    cmds.textField('variantNameText', edit=True, visible=(selectedItem == createNew))

def variantNameTextChanged(variantName):
    # The text field cannot be empty. Reset to default value if it is.
    if not variantName:
        cmds.textField('variantNameText', edit=True, text=kDefaultCacheVariantName)
    else:
        # Make sure the name user entered doesn't contain any invalid characters.
        validatedName = Tf.MakeValidIdentifier(variantName)
        if validatedName != variantName:
            cmds.textField('variantNameText', edit=True, text=validatedName)

def primNameTextChanged(primName):
    # The text field cannot be empty. Reset to default value if it is.
    mayaRefPrimParent = _pulledMayaRefPrim.GetParent()
    if not primName:
        primName = mayaUsd.ufe.uniqueChildName(mayaRefPrimParent, kDefaultCachePrimName)
        cmds.textFieldGrp('primNameText', edit=True, text=primName)
    else:
        # Make sure the name user entered is unique and doesn't contain
        # any invalid characters.
        validatedName = Tf.MakeValidIdentifier(primName)
        validatedName = mayaUsd.ufe.uniqueChildName(mayaRefPrimParent, validatedName)
        if validatedName != primName:
            cmds.textFieldGrp('primNameText', edit=True, text=validatedName)

def cacheFileUsdHierarchyOptions(topForm):
    '''Create controls to insert Maya reference USD cache into USD hierarchy.'''

    cmds.setParent(topForm)
    cmds.frameLayout(label=getMayaUsdLibString("kCacheMayaRefUsdHierarchy"))
    widgetColumn = cmds.columnLayout()

    rl = mel.eval('createRowLayoutforMayaReference("' + widgetColumn + '", "cacheFilePreviewRow", 2)')
    with mayaRefUtils.SetParentContext(rl):
        cmds.iconTextStaticLabel(st="iconAndTextHorizontal", i1="info.png",
                                 l=getMayaUsdLibString('kCacheFileWillAppear'))
        cmds.textField(text=str(_pulledMayaRefPrim.GetParent().GetPath()), editable=False)

    with mayaRefUtils.SetParentContext(cmds.rowLayout(numberOfColumns=2)):
        cmds.optionMenuGrp('compositionArcTypeMenu',
                           label=getMayaUsdLibString('kOptionAsCompositionArc'),
                           cc=compositionArcChanged)
        cmds.menuItem(label=getMayaUsdLibString('kMenuPayload'))
        cmds.menuItem(label=getMayaUsdLibString('kMenuReference'))
        cmds.optionMenu('listEditedAsMenu',
                        label=getMayaUsdLibString('kOptionListEditedAs'),
                        cc=listEditChanged)
        cmds.menuItem(label=getMayaUsdLibString('kMenuAppend'))
        cmds.menuItem(label=getMayaUsdLibString('kMenuPrepend'))

    variantRb = cmds.radioButtonGrp('variantRadioBtn',
                                    nrb=1,
                                    label=getMayaUsdLibString('kTextDefineIn'),
                                    l1=getMayaUsdLibString('kTextVariant'))

    rl = mel.eval('createRowLayoutforMayaReference("' + widgetColumn + '", "usdCacheVariantSetRow", 3)')
    with mayaRefUtils.SetParentContext(rl):
        cmds.text(label=getMayaUsdLibString('kMayaRefVariantSetName'))
        cmds.optionMenu('variantSetMenu',
                        cc=variantSetNameChanged)
        cmds.textField(visible=False)

    rl = mel.eval('createRowLayoutforMayaReference("' + widgetColumn + '", "usdCacheVariantNameRow", 3)')
    with mayaRefUtils.SetParentContext(rl):
        cmds.text(label=getMayaUsdLibString('kMayaRefVariantName'))
        cmds.optionMenu('variantNameMenu',
                        cc=variantNameChanged)
        cmds.textField('variantNameText',
                       cc=variantNameTextChanged)

    newChildRb = cmds.radioButtonGrp('newChildPrimRadioBtn',
                                     nrb=1,
                                     label='',
                                     scl=variantRb,
                                     l1=getMayaUsdLibString('kButtonNewChildPrim'))

    cmds.textFieldGrp('primNameText',
                      label=getMayaUsdLibString('kMayaRefPrimName'),
                      cc=primNameTextChanged)

    cmds.radioButtonGrp(variantRb, edit=True, select=1)

# Adapted from fileOptions.mel:fileOptionsTabPage().
def fileOptionsTabPage(tabLayout):
    mayaRefUtils.pushOptionsUITemplate()

    cmds.setParent(tabLayout)

    # See fileOptions.mel:fileOptionsTabPage() comments.  Contrary to that
    # code, we are not keeping 'optionsBoxForm' as a global MEL variable.  We
    # assume that we don't need to reference this directly to hide UI
    # temporarily.
    optBoxForm = cmds.formLayout('optionsBoxForm')

    topFrame = cmds.frameLayout(
        'optionsBoxFrame', collapsable=False, labelVisible=False,
        marginWidth=10, borderVisible=False)
    cmds.formLayout(optBoxForm, edit=True, af=[(topFrame, 'left', 0),
                    (topFrame, 'top', 0), (topFrame, 'right', 0),
                    (topFrame, 'bottom', 0)])

    topForm = cmds.columnLayout('actionOptionsForm', rowSpacing=5)
    
    cmds.setParent(topForm)
    cmds.frameLayout(label=getMayaUsdLibString("kMayaRefDescription"))
    cmds.columnLayout(adjustableColumn=True)
    cmds.text(align="left", label="TBD")

    cmds.setParent(topForm)
    cmds.frameLayout(label=getMayaUsdLibString("kCacheMayaRefOptions"))

    # USD file option controls will be parented under this layout.
    # resultCallback not called on "post", is therefore an empty string.
    fileOptionsScroll = cmds.columnLayout('fileOptionsScroll')
    mel.eval('mayaUsdTranslatorExport("fileOptionsScroll", "post={exportOpts}", "{cacheOpts}", "")'.format(exportOpts=kTranslatorExportOptions, cacheOpts=getCacheExportOptions()))

    cacheFileUsdHierarchyOptions(topForm)

    cmds.setUITemplate(popTemplate=True)

def getCacheExportOptions():
    global _cacheExportOptions
    if _cacheExportOptions is None:
        _cacheExportOptions = cacheToUsd.getDefaultExportOptions()
    return _cacheExportOptions

def setCacheOptions(newCacheOptions):
    global _cacheExportOptions
    # Animation is always on for cache to USD.
    _cacheExportOptions = cacheToUsd.turnOnAnimation(newCacheOptions)

def cacheCreateUi(parent):
    cmds.setParent(parent)
    fileOptionsTabPage(cmds.scrollLayout(childResizable=True))

def cacheInitUi(parent, filterType):
    cmds.setParent(parent)
    
    # If the parent of the Maya reference prim has a variant set, define in
    # variant is the default, otherwise all variant options are disabled.
    mayaRefPrimParent = _pulledMayaRefPrim.GetParent()

    if mayaRefPrimParent.HasVariantSets():
        # Define in variant is the default.
        cmds.radioButtonGrp('variantRadioBtn', edit=True, select=1)
        variantSets = mayaRefPrimParent.GetVariantSets()
        variantSetsNames = variantSets.GetNames()
        for vsName in variantSetsNames:
            cmds.menuItem(label=vsName, parent='variantSetMenu')
        cmds.optionMenu('variantSetMenu', edit=True, select=1)

        # If there is a variant set name 'Representation' then
        # automatically select it, otherwise select the first one.
        if mayaRefUtils.defaultVariantSetName() in variantSetsNames:
            variantSetNameChanged(mayaRefUtils.defaultVariantSetName())
        else:
            variantSetNameChanged(variantSetsNames[0])
    else:
        # No variant sets: disable all variant-related controls.
        cmds.radioButtonGrp('variantRadioBtn', edit=True, enable=False)
        cmds.rowLayout('usdCacheVariantSetRow', edit=True, enable=False)
        cmds.rowLayout('usdCacheVariantNameRow', edit=True, enable=False)

        cmds.radioButtonGrp('newChildPrimRadioBtn', edit=True, select=1)

    # Set initial (unique) name for child prim.
    primName = mayaUsd.ufe.uniqueChildName(mayaRefPrimParent, kDefaultCachePrimName)
    cmds.textFieldGrp('primNameText', edit=True, text=primName)

def cacheCommitUi(parent, selectedFile):
    # Read data to set up cache.

    # The following call will set _cacheExportOptions.  Initial settings not
    # accessed on "query", is therefore an empty string.
    mel.eval('mayaUsdTranslatorExport("fileOptionsScroll", "query={exportOpts}", "", "mayaUsdCacheMayaReference_setCacheOptions")'.format(exportOpts=kTranslatorExportOptions))

    primName = cmds.textFieldGrp('primNameText', query=True, text=True)
    payloadOrReference = cmds.optionMenuGrp('compositionArcTypeMenu', query=True, value=True)
    listEditType = cmds.optionMenu('listEditedAsMenu', query=True, value=True)
    
    defineInVariant = cmds.radioButtonGrp('variantRadioBtn', query=True, select=True)
    if defineInVariant:
        variantSetName = cmds.optionMenu('variantSetMenu', query=True, value=True)
        variantName = cmds.optionMenu('variantNameMenu', query=True, value=True)
        if variantName == 'Create New':
            variantName = cmds.textField('variantNameText', query=True, text=True)
    else:
        variantName = None
        variantSetName = None

    userArgs = cacheToUsd.getCacheCreationOptions(
        getCacheExportOptions(), selectedFile, primName, payloadOrReference,
        listEditType, variantSetName, variantName)

    # Call push.
    if not mayaUsd.lib.PrimUpdaterManager.mergeToUsd(_mayaRefDagPath, userArgs):
        errorMsgFormat = getMayaUsdLibString('kErrorCacheToUsdFailed')
        errorMsg = cmds.format(errorMsgFormat, stringArg=(_mayaRefDagPath))
        cmds.error(errorMsg)

def cacheDialog(dagPath, pulledMayaRefPrim, _):
    '''Display dialog to cache the argument pulled Maya reference prim to USD.'''

    global _mayaRefDagPath
    global _pulledMayaRefPrim
    
    _mayaRefDagPath = dagPath
    _pulledMayaRefPrim = pulledMayaRefPrim

    ok = getMayaUsdLibString('kCacheMayaRefCache')
    fileFilter = getMayaUsdLibString("kAllUsdFiles") + " (*.usd *.usda *.usdc *.usdz);;*.usd;;*.usda;;*.usdc;;*.usdz";

    # As per Maya projectViewer.mel code structure, the UI creation
    # (optionsUICreate) creates the main UI framework, and UI initialization
    # (optionsUIInit) puts up the export options.
    files = cmds.fileDialog2(
        returnFilter=1, fileMode=0, caption=getMayaUsdLibString('kCaptionCacheToUsd'), 
        fileFilter=fileFilter, dialogStyle=2, okCaption=ok, 
        # The callbacks below must be MEL procedures, as per fileDialog2
        # requirements.  They call their corresponding Python functions.
        optionsUICreate="mayaUsdCacheMayaReference_cacheCreateUi",
        optionsUIInit="mayaUsdCacheMayaReference_cacheInitUi",
        optionsUITitle="",
        optionsUICommit2="mayaUsdCacheMayaReference_cacheCommitUi",
        startingDirectory=cmds.workspace(query=True, directory=True)
    )
