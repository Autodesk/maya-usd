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

from mayaUsdLibRegisterStrings import getMayaUsdLibString
import mayaUsdOptions

from functools import partial


_kDuplicateAsMayaDataOptionsHelpContentId = 'UsdEditDuplicateAsMaya'

def showDuplicateAsMayaDataOptions():
    """
    Shows the duplicate-as-Maya-data options dialog.
    """

    windowName = "DuplicateAsMayaDataOptionsDialog"
    if cmds.window(windowName, query=True, exists=True):
        if cmds.window(windowName, query=True, visible=True):
            return
        # If it exists but is not visible, assume something wrong happened.
        # Delete the window and recreate it.
        cmds.deleteUI(windowName)

    window = cmds.window(windowName, title=getMayaUsdLibString("kDuplicateAsMayaDataOptionsTitle"), widthHeight=mayaUsdOptions.defaultOptionBoxSize())
    _createDuplicateAsMayaDataOptionsDialog(window)


def _createDuplicateAsMayaDataOptionsDialog(window):
    """
    Creates the duplicate-as-Maya-data dialog.
    """

    windowLayout = cmds.setParent(query=True)

    menuBarLayout = cmds.menuBarLayout(parent=windowLayout)

    windowFormLayout = cmds.formLayout(parent=windowLayout)

    subFormLayout = cmds.formLayout(parent=windowFormLayout)
    subScrollLayout = cmds.scrollLayout('DuplicateAsMayaDataOptionsDialogSubScrollLayout', cr=True, bv=True)
    cmds.formLayout(subFormLayout, edit=True,
        attachForm=[
            (subScrollLayout,       "top",      0),
            (subScrollLayout,       "bottom",   0),
            (subScrollLayout,       "left",     0),
            (subScrollLayout,       "right",    0)]);
    subLayout = cmds.columnLayout("DuplicateAsMayaDataOptionsDialogSubLayout", adjustableColumn=True)

    optionsText = getDuplicateAsMayaDataOptionsText()
    _fillDuplicateAsMayaDataOptionsDialog(subLayout, optionsText, "post")

    menu = cmds.menu(label=getMayaUsdLibString("kEditMenu"), parent=menuBarLayout)
    cmds.menuItem(label=getMayaUsdLibString("kSaveSettingsMenuItem"), command=_saveDuplicateAsMayaDataOptions)
    cmds.menuItem(label=getMayaUsdLibString("kResetSettingsMenuItem"), command=partial(_resetDuplicateAsMayaDataOptions, subLayout))

    cmds.setParent(menuBarLayout)
    if _hasDuplicateAsMayaDataOptionsHelp():
        menu = cmds.menu(label=getMayaUsdLibString("kHelpMenu"), parent=menuBarLayout)
        cmds.menuItem(label=getMayaUsdLibString("kHelpDuplicateAsMayaDataOptionsMenuItem"), command=_helpDuplicateAsMayaDataOptions)

    buttonsLayout = cmds.formLayout(parent=windowFormLayout)

    applyText  = getMayaUsdLibString("kApplyButton")
    closeText = getMayaUsdLibString("kCloseButton")
    # Use same height for buttons as in Maya option boxes.
    bApply     = cmds.button(label=applyText, width=100, height=26, command=_saveDuplicateAsMayaDataOptions)
    bClose     = cmds.button(label=closeText, width=100, height=26, command=partial(_closeDuplicateAsMayaDataOptionsDialog, window))

    spacer = 8      # Same spacing as Maya option boxes.
    edge   = 6

    cmds.formLayout(buttonsLayout, edit=True,
        attachForm=[
            (bClose,    "bottom", edge),
            (bClose,    "right",  edge),

            (bApply,    "bottom", edge),
            (bApply,    "left",   edge),
        ],
        attachPosition=[
            (bClose,    "left",   spacer/2, 50),
            (bApply,    "right",  spacer/2, 50),
        ])

    cmds.formLayout(windowFormLayout, edit=True,
        attachForm=[
            (subFormLayout,         'top',      0),
            (subFormLayout,         'left',     0),
            (subFormLayout,         'right',    0),
            
            (buttonsLayout,         'left',     0),
            (buttonsLayout,         'right',    0),
            (buttonsLayout,         'bottom',   0),
        ],
        attachControl=[
            (subFormLayout,         'bottom',   edge, buttonsLayout)
        ])

    cmds.showWindow()


def _closeDuplicateAsMayaDataOptionsDialog(window, data=None):
    """
    Reacts to the duplicate-as-Maya-data options dialog being closed by the user.
    """
    cmds.deleteUI(window)


def _saveDuplicateAsMayaDataOptions(data=None):
    """
    Saves the duplicate-as-Maya-data options as currently set in the dialog.
    The MEL receiveDuplicateAsMayaDataOptionsTextFromDialog will call setDuplicateAsMayaDataOptionsText.
    """
    mel.eval('''mayaUsdTranslatorExport("", "query=all;!output", "", "receiveDuplicateAsMayaDataOptionsTextFromDialog");''')


def _resetDuplicateAsMayaDataOptions(subLayout, data=None):
    """
    Resets the duplicate-as-Maya-data options in the dialog.
    """
    optionsText = mayaUsdOptions.convertOptionsDictToText(_getDefaultDuplicateAsMayaDataOptionsDict())
    _fillDuplicateAsMayaDataOptionsDialog(subLayout, optionsText, "fill")


def _fillDuplicateAsMayaDataOptionsDialog(subLayout, optionsText, action):
    """
    Fills the duplicate-as-Maya-data options dialog UI elements with the given options.
    """
    cmds.setParent(subLayout)
    mel.eval(
        '''
        mayaUsdTranslatorExport("{subLayout}", "{action}=all;!output", "{optionsText}", "")
        '''.format(optionsText=optionsText, subLayout=subLayout, action=action))


def _hasDuplicateAsMayaDataOptionsHelp():
    """
    Returns True if the help topic for the options is available in Maya.
    """
    # Note: catchQuiet returns 0 or 1, not the value, so we use a dummy assignment
    #       to produce the value to be returned by eval().
    url = mel.eval('''catchQuiet($url = `showHelp -q "''' + _kDuplicateAsMayaDataOptionsHelpContentId + '''"`); $a = $url;''')
    return bool(url)

def _helpDuplicateAsMayaDataOptions(data=None):
    """
    Shows help on the duplicate-as-Maya-data options dialog.
    """
    cmds.showHelp(_kDuplicateAsMayaDataOptionsHelpContentId)


def _getDuplicateAsMayaDataOptionsVarName():
    """
    Retrieves the option var name under which the duplicate-as-Maya-data options are kept.
    """
    return "mayaUsd_DuplicateAsMayaDataOptions"


def getDuplicateAsMayaDataOptionsText():
    """
    Retrieves the current duplicate-as-Maya-data options as text with column-spearated key/value pairs.
    """
    return mayaUsdOptions.getOptionsText(
        _getDuplicateAsMayaDataOptionsVarName(),
        _getDefaultDuplicateAsMayaDataOptionsDict())
    

def setDuplicateAsMayaDataOptionsText(optionsText):
    """
    Sets the current duplicate-as-Maya-data options as text with column-spearated key/value pairs.
    Called via receiveDuplicateAsMayaDataOptionsTextFromDialog in MEL, which gets called via
    mayaUsdTranslatorExport in query mode.
    """
    mayaUsdOptions.setOptionsText(_getDuplicateAsMayaDataOptionsVarName(), optionsText)


def _getDefaultDuplicateAsMayaDataOptionsDict():
    """
    Retrieves the default duplicate-as-Maya-data options.
    """
    return {
        "exportColorSets":          "1",
        "exportUVs":                "1",
        "exportSkels":              "none",
        "exportSkin":               "none",
        "exportBlendShapes":        "0",
        "exportDisplayColor":       "1",
        "shadingMode":              "none",
        "animation":                "1",
        "exportVisibility":         "1",
        "exportInstances":          "1",
        "mergeTransformAndShape":   "1",
        "stripNamespaces":          "0",
    }
