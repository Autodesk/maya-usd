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
from mayaUsdLibRegisterStrings import getMayaUsdLibString
from pxr import Sdf

# These names should not be localized as Usd only accepts [a-z,A-Z] as valid characters.
kDefaultMayaReferencePrimName = 'MayaReference1'
kDefaultVariantSetName = 'Representation'
kDefaultVariantName = 'MayaReference'

compositionArcKey = 'compositionArc'
compositionArcPayload = 'Payload'
compositionArcReference = 'Reference'
_compositionArcValues = [compositionArcReference, compositionArcPayload]

listEditTypeKey = 'listEditType'
listEditTypePrepend = 'Prepend'
listEditTypeAppend = 'Append'
_listEditedAsValues = [listEditTypePrepend, listEditTypeAppend]

loadPayloadKey = 'loadPayload'

referencedPrimPathKey = 'referencedPrimPath'

def defaultMayaReferencePrimName():
    return kDefaultMayaReferencePrimName

def defaultVariantSetName():
    return kDefaultVariantSetName

def defaultVariantName():
    return kDefaultVariantName

class SetParentContext():
    '''Simple context helper to go up one parent level when exiting.'''
    def __init__(self, parent):
        cmds.setParent(parent)
        pass

    def __enter__(self):
        pass

    def __exit__(self, mytype, value, tb):
        cmds.setParent('..')

def pushOptionsUITemplate():
    '''Standardize the look of the options UI.

    Python translation of fileOptions.mel:pushOptionsUITemplate(),
    which is not a global proc.
    '''
    if not cmds.uiTemplate('optionsTemplate', exists=True):
        cmds.uiTemplate('optionsTemplate')

        cmds.frameLayout(defineTemplate='optionsTemplate',
                         collapsable=True,
                         collapse=False,
                         labelVisible=True,
                         labelIndent=5,
                         marginWidth=5,
                         marginHeight=5)

        cmds.columnLayout(defineTemplate='optionsTemplate',
                          adjustableColumn=True)

    cmds.setUITemplate('optionsTemplate', pushTemplate=True)

def _getMenuIndex(values, current, defaultIndex = 1):
    """
    Retrieves the menu index corresponding to the current value selected amongst values.
    If the value is invalid, returns the defaultIndex.
    """
    try:
        # Note: menu index is 1-based.
        return values.index(current) + 1
    except:
        return defaultIndex

def _getMenuGrpValue(menuName, values, defaultIndex = 0):
    """
    Retrieves the currently selected values from a menu.
    """
    # Note: option menu selection index start at 1, so we subtract 1.
    menuIndex = cmds.optionMenuGrp(menuName, query=True, select=True) - 1
    if 0 <= menuIndex < len(values):
        return values[menuIndex]
    else:
        return values[defaultIndex]

def _getMenuValue(menuName, values, defaultIndex = 0):
    """
    Retrieves the currently selected values from a menu.
    """
    # Note: option menu selection index start at 1, so we subtract 1.
    menuIndex = cmds.optionMenu(menuName, query=True, select=True) - 1
    if 0 <= menuIndex < len(values):
        return values[menuIndex]
    else:
        return values[defaultIndex]

def _compositionArcChanged(selectedItem):
    """
    Reacts to the composition arc type being selected by the user.
    """
    compositionArc = _getMenuGrpValue('compositionArcTypeMenu', _compositionArcValues)
    enableLoadPayload = bool(compositionArc == compositionArcPayload)
    cmds.checkBoxGrp('loadPayload', edit=True, enable=enableLoadPayload)

def _selectReferencedPrim(*args):
    """
    Open a dialog to select a prim from the currently selected file
    to be the referenced prim.
    """
    filename = _getCurrentFilename()
    primPath = cmds.textFieldGrp('mayaUsdAddRefOrPayloadPrimPath', query=True, text=True)
    title = getMayaUsdLibString('kAddRefOrPayloadPrimPathTitle')
    helpLabel = getMayaUsdLibString('kAddRefOrPayloadPrimPathHelpLabel')
    helpToken = 'UsdHierarchyView' # TODO: real help ID for the referenced prim dialog.
    result = cmds.usdImportDialog(filename, hideVariants=True, hideRoot=True, primPath=primPath, title=title, helpLabel=helpLabel, helpToken=helpToken)
    if result:
        primPath = cmds.usdImportDialog(query=True, primPath=True)
        cmds.textFieldGrp('mayaUsdAddRefOrPayloadPrimPath', edit=True, text=primPath)
    cmds.usdImportDialog(clearData=True)

_currentFileName = None

def _setCurrentFilename(filename):
    """Sets the current file selection."""
    global _currentFileName
    _currentFileName = filename

def _getCurrentFilename():
    """Retrieve the current file selection."""
    global _currentFileName
    return _currentFileName

def _resetReferencedPrim(*args):
    """Reset the referenced prim UI"""
    _updateReferencedPrimBasedOnFile()

# Note: we are disabling the default prim filling due to the long delay when the USD
#       file is very large.
#
# def _getDefaultAndRootPrims(filename):
#     """Retrieve the default and first root prims of a USD file."""
#     defPrim, rootPrim = None, None
#     try:
#         layer = Sdf.Layer.FindOrOpen(filename)
#         if layer:
#             # Note: the root prims at the USD layer level are SdfPrimSpec,
#             #       so they are not SdfPath themselves nor prim. That is
#             #       why their path is retrieved via their path property.
#             #
#             #       The default prim is a pure token though, because it is
#             #       a metadata on the layer, so it can be used as-is.
#             rootPrims = layer.rootPrims
#             rootPrim = rootPrims[0].path if len(rootPrims) > 0 else None
#             defPrim = layer.defaultPrim
#     except Exception as ex:
#         print(str(ex))
#
#     return defPrim, rootPrim

def _updateReferencedPrimBasedOnFile():
    """Update all UI related to the referenced prim based on the currently selected file."""
    filename = _getCurrentFilename()
    if not filename:
        cmds.textFieldGrp('mayaUsdAddRefOrPayloadPrimPath', edit=True, text='', placeholderText='')
        cmds.textFieldGrp('mayaUsdAddRefOrPayloadPrimPath', edit=True, enable=False)
        cmds.button('mayaUsdAddRefOrPayloadFilePathBrowser', edit=True, enable=False)
        cmds.symbolButton('mayaUsdAddRefOrPayloadFilePathReset', edit=True, enable=False)
        return

    cmds.textFieldGrp('mayaUsdAddRefOrPayloadPrimPath', edit=True, enable=True)
    cmds.button('mayaUsdAddRefOrPayloadFilePathBrowser', edit=True, enable=True)
    cmds.symbolButton('mayaUsdAddRefOrPayloadFilePathReset', edit=True, enable=True)

    # Note: we are disabling the default prim filling due to the long delay when the USD
    #       file is very large.
    #
    # defaultPrim, rootPrim = _getDefaultAndRootPrims(filename)
    # if defaultPrim:
    #     placeHolder = defaultPrim + getMayaUsdLibString('kAddRefOrPayloadPrimPathPlaceHolder')
    #     cmds.textFieldGrp('mayaUsdAddRefOrPayloadPrimPath', edit=True, text='', placeholderText=placeHolder)
    # elif rootPrim:
    #     cmds.textFieldGrp('mayaUsdAddRefOrPayloadPrimPath', edit=True, text=rootPrim, placeholderText='')
    # else:
    cmds.textFieldGrp('mayaUsdAddRefOrPayloadPrimPath', edit=True, text='', placeholderText='')

def createUsdRefOrPayloadUI(uiForLoad=False):
    _setCurrentFilename(None)

    with SetParentContext(cmds.rowLayout(numberOfColumns=2)):
        tooltip = getMayaUsdLibString('kOptionAsUSDReferenceToolTip')
        cmds.optionMenuGrp('compositionArcTypeMenu',
                           label=getMayaUsdLibString('kOptionAsUSDReference'),
                           annotation=tooltip,
                           statusBarMessage=getMayaUsdLibString('kOptionAsUSDReferenceStatusMsg'))
        compositionArcLabels = [getMayaUsdLibString('kMenuReference'),getMayaUsdLibString('kMenuPayload')]
        for label in compositionArcLabels:
            cmds.menuItem(label=label)
        cmds.optionMenu('listEditedAsMenu',
                        label=getMayaUsdLibString('kOptionListEditedAs'),
                        annotation=tooltip,
                        statusBarMessage=getMayaUsdLibString('kOptionAsUSDReferenceStatusMsg'))
        listEditedAsLabels = [getMayaUsdLibString('kMenuPrepend'), getMayaUsdLibString('kMenuAppend')]
        for label in listEditedAsLabels:
            cmds.menuItem(label=label)

    if uiForLoad:
        with SetParentContext(cmds.rowLayout(numberOfColumns=3, adjustableColumn1=True)):
            tooltip = getMayaUsdLibString('kAddRefOrPayloadPrimPathToolTip')
            primPathLabel = getMayaUsdLibString("kAddRefOrPayloadPrimPathLabel")
            selectLabel = getMayaUsdLibString("kAddRefOrPayloadSelectLabel")
            cmds.textFieldGrp('mayaUsdAddRefOrPayloadPrimPath', label=primPathLabel, ann=tooltip)
            cmds.button('mayaUsdAddRefOrPayloadFilePathBrowser', label=selectLabel, ann=tooltip)
            cmds.symbolButton('mayaUsdAddRefOrPayloadFilePathReset', image="refresh.png", ann=tooltip)

        cmds.checkBoxGrp('loadPayload',
                         label=getMayaUsdLibString('kOptionLoadPayload'),
                         annotation=getMayaUsdLibString('kLoadPayloadAnnotation'),
                         ncb=1)

def initUsdRefOrPayloadUI(values, uiForLoad=False):
    compositionArcMenuIndex = _getMenuIndex(_compositionArcValues, values[compositionArcKey])
    cmds.optionMenuGrp('compositionArcTypeMenu', edit=True, select=compositionArcMenuIndex)

    listEditTypeMenuIndex = _getMenuIndex(_listEditedAsValues,values[listEditTypeKey])
    cmds.optionMenu('listEditedAsMenu', edit=True, select=listEditTypeMenuIndex)

    if uiForLoad:
        cmds.optionMenuGrp('compositionArcTypeMenu', edit=True, cc=_compositionArcChanged)
        _compositionArcChanged(compositionArcMenuIndex)
        cmds.checkBoxGrp('loadPayload', edit=True, value1=values[loadPayloadKey])

        cmds.button("mayaUsdAddRefOrPayloadFilePathBrowser", c=_selectReferencedPrim, edit=True)
        cmds.button("mayaUsdAddRefOrPayloadFilePathReset", c=_resetReferencedPrim, edit=True)

        _updateReferencedPrimBasedOnFile()

def updateUsdRefOrPayloadUI(selectedFile):
    if selectedFile == _getCurrentFilename():
        return
    _setCurrentFilename(selectedFile)
    _updateReferencedPrimBasedOnFile()

def commitUsdRefOrPayloadUI(uiForLoad=False):
    values = {}
    values[compositionArcKey] = _getMenuGrpValue('compositionArcTypeMenu', _compositionArcValues)
    values[listEditTypeKey  ] = _getMenuValue('listEditedAsMenu', _listEditedAsValues)
    values[loadPayloadKey   ] = cmds.checkBoxGrp('loadPayload', query=True, value1=True) if uiForLoad else True
    values[referencedPrimPathKey] = cmds.textFieldGrp('mayaUsdAddRefOrPayloadPrimPath', query=True, text=True) if uiForLoad else ''

    return values
