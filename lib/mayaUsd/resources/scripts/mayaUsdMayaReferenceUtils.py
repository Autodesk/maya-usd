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

# These names should not be localized as Usd only accepts [a-z,A-Z] as valid characters.
kDefaultMayaReferencePrimName = 'MayaReference1'
kDefaultVariantSetName = 'Representation'
kDefaultVariantName = 'MayaReference'

compositionArcKey = 'compositionArc'
compositionArcPayload = 'Payload'
compositionArcReference = 'Reference'
_compositionArcValues = [compositionArcPayload, compositionArcReference]

listEditTypeKey = ''
listEditTypeAppend = 'Append'
listEditTypePrepend = 'Prepend'
_listEditedAsValues = [listEditTypeAppend, listEditTypePrepend]

loadPayloadKey = 'loadPayload'

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

def createUsdRefOrPayloadUI(showLoadPayload=False):
    with SetParentContext(cmds.rowLayout(numberOfColumns=2)):
        tooltip = getMayaUsdLibString('kOptionAsUSDReferenceToolTip')
        cmds.optionMenuGrp('compositionArcTypeMenu',
                           label=getMayaUsdLibString('kOptionAsUSDReference'),
                           annotation=tooltip)
        compositionArcLabels = [getMayaUsdLibString('kMenuPayload'), getMayaUsdLibString('kMenuReference')]
        for label in compositionArcLabels:
            cmds.menuItem(label=label)
        cmds.optionMenu('listEditedAsMenu',
                        label=getMayaUsdLibString('kOptionListEditedAs'),
                        annotation=tooltip)
        listEditedAsLabels = [getMayaUsdLibString('kMenuAppend'), getMayaUsdLibString('kMenuPrepend')]
        for label in listEditedAsLabels:
            cmds.menuItem(label=label)

    if showLoadPayload:
        cmds.checkBoxGrp('loadPayload',
                         label=getMayaUsdLibString('kOptionLoadPayload'),
                         ncb=1)

def initUsdRefOrPayloadUI(values, showLoadPayload=False):
    compositionArcMenuIndex = _getMenuIndex(_compositionArcValues, values[compositionArcKey])
    cmds.optionMenuGrp('compositionArcTypeMenu', edit=True, select=compositionArcMenuIndex)

    listEditTypeMenuIndex = _getMenuIndex(_listEditedAsValues,values[listEditTypeKey])
    cmds.optionMenu('listEditedAsMenu', edit=True, select=listEditTypeMenuIndex)

    if showLoadPayload:
        cmds.optionMenuGrp('compositionArcTypeMenu', edit=True, cc=_compositionArcChanged)
        _compositionArcChanged(compositionArcMenuIndex)
        cmds.checkBoxGrp('loadPayload', edit=True, value1=values[loadPayloadKey])

def commitUsdRefOrPayloadUI(showLoadPayload=False):
    values = {}
    values[compositionArcKey] = _getMenuGrpValue('compositionArcTypeMenu', _compositionArcValues)
    values[listEditTypeKey  ] = _getMenuValue('listEditedAsMenu', _listEditedAsValues)
    values[loadPayloadKey   ] = cmds.checkBoxGrp('loadPayload', query=True, value1=True) if showLoadPayload else True
    return values
