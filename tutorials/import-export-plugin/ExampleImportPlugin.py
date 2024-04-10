import mayaUsd.lib
import maya.OpenMayaUI as mui
import maya.cmds as cmds

from pxr import Plug, Tf

import json

try:
    from shiboken2 import wrapInstance
    import PySide2.QtWidgets as QtW
    import PySide2.QtGui as QtG
except Exception:
    from shiboken6 import wrapInstance
    import PySide6.QtWidgets as QtW
    import PySide6.QtGui as QtG


# Importer plugin description: job context, UI name, description.
exampleJobContextName   = 'ExampleImportPlugin'
exampleFriendlyName     = 'This is the example import plugin'
exampleDescription      = 'The example import plugin will import this or that'


# Importer plugin settings. Saved on-disk using a Maya option variable.
#
# Note: we are controlling the import-instances setting even though that
#       setting already has a UI in the MayaUSD export dialog. When
#       an existing setting is forced in this way, the UI in the main
#       dialog is disabled.
#
#       We recommend *not* controlling existing settings. We support
#       it in case your plugin has special needs that require forcing
#       the setting to a specific value. For example to ensure it is
#       within a special range or that it is always on or always off.
exampleOptionVar = 'ExampleImportPluginOptionVar'
importInstancesToken = 'importInstances'
customImportSettingToken = 'yourCustomImportSetting'
exampleSettings = {
    importInstancesToken: True,
    customImportSettingToken: 33,
}

def logDebug(msg):
    '''
    To help trace what is going on.
    '''
    print(msg)
    pass

def loadSettings():
    '''
    Load the importing plugin settings from a Maya option var, if it exists.
    '''
    if cmds.optionVar(exists=exampleOptionVar):
         global exampleSettings
         exampleSettings.update(json.loads(cmds.optionVar(query=exampleOptionVar)))


def saveSettings():
    '''
    Save the importing plugin settings in a Maya option var.
    '''
    global exampleSettings
    cmds.optionVar(stringValue=(exampleOptionVar, json.dumps(exampleSettings)))


def tokenToLabel(token):
    '''
    Utility function to convert a token name into a somehwat nice UI label.
    Adds a space before all uppercase letters and capitalizes the label.
    '''
    import re
    return re.sub('[A-Z]', lambda m: ' ' + m.group(0), token).capitalize()


def fillUI(jobContext, settings):
    '''
    Importer plugin UI creation and filling function.
    '''
    windowLayout = cmds.setParent(query=True)
    windowFormLayout = cmds.formLayout(parent=windowLayout)

    controlsLayout = cmds.columnLayout(parent=windowFormLayout)

    cmds.setParent(controlsLayout)

    cmds.text(label='These are the import-plugin settings.\n', font='boldLabelFont')

    importInstances = True
    if importInstancesToken in settings:
        importInstances = settings[importInstancesToken]
    cmds.checkBoxGrp(importInstancesToken, label=tokenToLabel(importInstancesToken), value1=bool(importInstances))

    customSetting = 33
    if customImportSettingToken in settings:
        customSetting = settings[customImportSettingToken]
    cmds.intFieldGrp(customImportSettingToken, label=tokenToLabel(customImportSettingToken), value1=int(customSetting))

    buttonsLayout = cmds.formLayout(parent=windowFormLayout)

    def cancel(data=None):
        cmds.layoutDialog(dismiss='cancel')

    def accept(data=None):
        queryUI(jobContext, settings, controlsLayout)
        saveSettings()
        cmds.layoutDialog(dismiss='accept')

    buttonWidth = 100
    buttonHeight = 26

    okButton = cmds.button(label='OK',  width=buttonWidth, height=buttonHeight, command=accept)
    cancelButton = cmds.button(label='Cancel', width=buttonWidth, height=buttonHeight, command=cancel)

    spacer = 8
    edge   = 6

    cmds.formLayout(buttonsLayout, edit=True,
        attachForm=[
            (cancelButton,  "bottom", edge),
            (cancelButton,  "right",  edge),
            (okButton,      "bottom", edge),
        ],
        attachControl=[
            (okButton,      "right", spacer, cancelButton),
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

def queryUI(jobContext, settings, container):
    '''
    Importer plugin UI query to retrieve the data when the UI is confirmed by the user.
    '''
    global exampleSettings

    importInstances = bool(cmds.checkBoxGrp(importInstancesToken, query=True, value1=True))
    exampleSettings[importInstancesToken] = importInstances
    settings[importInstancesToken] = importInstances

    customSetting = int(cmds.intFieldGrp(customImportSettingToken, query=True, value1=True))
    exampleSettings[customImportSettingToken] = customSetting
    settings[customImportSettingToken] = customSetting

def showUi(jobContext, parentUIName, settings):
    '''
    Generic function to show a dialog with OK/cancel buttons.
    '''
    def _fillUI():
        fillUI(jobContext, settings)

    try:
        cmds.layoutDialog(title="Options for %s" % jobContext, uiScript=_fillUI)
    except Exception as ex:
        logDebug("Error: " + str(ex))
    except:
        logDebug("Error")
    

def importUiFn(jobContext, parentUIName, settings):
    '''
    The importer plugin UI callback, shows a UI to edit the settings it want forced.
    '''
    # We want the export plugin specific settings to override the
    # input settings we received.
    global exampleSettings
    settings.update(exampleSettings)
    
    # logDebug("%s import plugin UI called for job context %s in %s with settings %s" %
    #       (exampleJobContextName, jobContext, parentUIName, str(settings)))
    
    showUi(jobContext, parentUIName, settings)

    # logDebug("%s import plugin UI returning: %s" % (exampleJobContextName, settings))

    return exampleSettings


def importEnablerFn():
    '''
    The importer plugin settings callback, returns the settings it want forced with the forced value.
    '''
    global exampleSettings
    # logDebug("%s import plugin settings called, returning: %s" %
    #       (exampleJobContextName, exampleSettings))
    return exampleSettings


def register():
    '''
    Register the importer plugin with an importer settings callback and a UI callback.
    '''
    logDebug("Registering the %s import plugin" % exampleJobContextName)
    mayaUsd.lib.JobContextRegistry.RegisterImportJobContext(
        exampleJobContextName, exampleFriendlyName, exampleDescription, importEnablerFn)

    mayaUsd.lib.JobContextRegistry.SetImportOptionsUI(
        exampleJobContextName, importUiFn)
    
    loadSettings()

