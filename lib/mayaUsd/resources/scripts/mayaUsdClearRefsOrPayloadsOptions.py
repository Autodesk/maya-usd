#
# Copyright 2023 Autodesk
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

from mayaUsdLibRegisterStrings import getMayaUsdLibString
import mayaUsdOptions

from functools import partial

kAllRefsCheckboxName = "allRefsCheckbox"
kAllPayloadsCheckboxName = "allPayloadsCheckbox"

_itemName = None

def showClearRefsOrPayloadsOptions(itemName):
    """
    Shows the clear references or payloads options dialog.
    """
    global _itemName
    _itemName = itemName
    title = getMayaUsdLibString("kClearRefsOrPayloadsOptionsTitle")
    userChoice = mel.eval('''layoutDialog -title "%s" -parent "MayaWindow" -uiScript createClearRefsOrPayloadsOptionsDialog;''' % title)

    results = [userChoice]

    options = getClearRefsOrPayloadsOptionsDict()
    for optionName in ['references', 'payloads']:
        if optionName in options and options[optionName]:
            results.append(optionName)

    return results


def _createClearRefsOrPayloadsOptionsDialog():
    """
    Creates the clear references or payloads dialog UI.
    """

    # Same spacing as Maya option boxes.
    spacer = 16
    edge   = 12

    # Use same height for buttons as in Maya option boxes.
    buttonWidth = 100
    buttonHeight = 26

    windowLayout = cmds.setParent(query=True)

    windowFormLayout = cmds.formLayout(parent=windowLayout)

    controlsLayout = cmds.formLayout(parent=windowFormLayout)

    cmds.setParent(controlsLayout)

    global _itemName
    messageLabel = cmds.text(label=getMayaUsdLibString("kClearRefsOrPayloadsOptionsMessage") % _itemName)

    subLayout = cmds.columnLayout("ClearRefsOrPayloadsOptionsDialogSubLayout", adjustableColumn=True)

    cmds.setParent(subLayout)

    cmds.checkBox(kAllRefsCheckboxName, label=getMayaUsdLibString("kAllRefsLabel"), annotation=getMayaUsdLibString("kAllRefsTooltip"))
    cmds.checkBox(kAllPayloadsCheckboxName, label=getMayaUsdLibString("kAllPayloadsLabel"), annotation=getMayaUsdLibString("kAllPayloadsTooltip"))

    buttonsLayout = cmds.formLayout(parent=windowFormLayout)

    clearCmd  = _acceptClearRefsOrPayloadsOptionsDialog
    cancelCmd = _cancelClearRefsOrPayloadsOptionsDialog
    clearButton  = cmds.button(label=getMayaUsdLibString("kClearButton"),  width=buttonWidth, height=buttonHeight, command=clearCmd)
    cancelButton = cmds.button(label=getMayaUsdLibString("kCancelButton"), width=buttonWidth, height=buttonHeight, command=cancelCmd)

    cmds.formLayout(buttonsLayout, edit=True,
        attachForm=[
            (cancelButton,   "bottom", edge),
            (cancelButton,   "right",  edge),
            (clearButton,    "bottom", edge),
        ],
        attachControl=[
            (clearButton,    "right", spacer, cancelButton),
        ])

    cmds.formLayout(controlsLayout, edit=True,
        attachForm=[
            (messageLabel,   "top",   edge),
            (messageLabel,   "left",  edge),
            (messageLabel,   "right", edge),
            (subLayout,      "left",  edge),
            (subLayout,      "right", edge),
        ],
        attachControl=[
            (subLayout,    "top", spacer, messageLabel),
        ])

    cmds.formLayout(windowFormLayout, edit=True,
        attachForm=[
            (controlsLayout, 'top',      0),
            (controlsLayout, 'left',     0),
            (controlsLayout, 'right',    0),
            
            (buttonsLayout,  'left',     0),
            (buttonsLayout,  'right',    0),
            (buttonsLayout,  'bottom',   0),
        ],
        attachControl=[
            (controlsLayout, 'bottom',   edge, buttonsLayout)
        ])
    
    _fillClearRefsOrPayloadsOptionsDialog()


def _fillClearRefsOrPayloadsOptionsDialog():
    """
    Fills the clear references or payloads options dialog UI elements with the current options.
    """
    options = getClearRefsOrPayloadsOptionsDict()
    cmds.checkBox(kAllRefsCheckboxName, edit=True, value=options['references'])
    cmds.checkBox(kAllPayloadsCheckboxName, edit=True, value=options['payloads'])


def _acceptClearRefsOrPayloadsOptionsDialog(data=None):
    """
    Reacts to the clear references or payloads options dialog being OK'ed by the user.
    """
    options = {
        'references' : bool(cmds.checkBox(kAllRefsCheckboxName, query=True, value=True)),
        'payloads'   : bool(cmds.checkBox(kAllPayloadsCheckboxName, query=True, value=True)),
    }
    setClearRefsOrPayloadsOptionsDict(options)
    cmds.layoutDialog(dismiss='Clear')


def _cancelClearRefsOrPayloadsOptionsDialog(data=None):
    """
    Reacts to the clear references or payloads options dialog being closed by the user.
    """
    cmds.layoutDialog(dismiss='dismiss')


###########################################################
#
# Option var handling below.

def _getClearRefsOrPayloadsOptionsVarName():
    """
    Retrieves the option var name under which the clear references or payloads options are kept.
    """
    return "mayaUsd_ClearRefsOrPayloadsOptions"

def getDefaultClearRefsOrPayloadsOptionsDict():
    """
    Retrieves the default clear references or payloads options dictionary.
    """
    return {
        "references": True,
        "payloads":   True,
    }

def getClearRefsOrPayloadsOptionsDict():
    """
    Retrieves the current clear references or payloads options as a dictionary.
    """
    return mayaUsdOptions.getOptionsDict(
        _getClearRefsOrPayloadsOptionsVarName(),
        getDefaultClearRefsOrPayloadsOptionsDict())

def setClearRefsOrPayloadsOptionsDict(options):
    """
    Sets the current clear references or payloads options from teh given dictionary.
    """
    mayaUsdOptions.setOptionsDict(
        _getClearRefsOrPayloadsOptionsVarName(),
        options)

