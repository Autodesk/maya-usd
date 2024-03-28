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


# Exporter plugin description: job context, UI name, description.
exampleJobContextName   = 'ExampleExportPlugin'
exampleFriendlyName     = 'This is the example export plugin'
exampleDescription      = 'The example export plugin will export this or that'


# Exporter plugin settings. Saved on-disk using a Maya option variable.
exampleOptionVar = 'ExampleExportPluginOptionVar'
strideToken = 'frameStride'
exampleSettings = {
    strideToken: 5.0,
}

def logDebug(msg):
    '''
    To help trace what is going on.
    '''
    print(msg)
    pass

def loadSettings():
    '''
    Load the exporting plugin settings from a Maya option var, if it exists.
    '''
    if cmds.optionVar(exists=exampleOptionVar):
         global exampleSettings
         exampleSettings = json.loads(cmds.optionVar(query=exampleOptionVar))


def saveSettings():
    '''
    Save the exporting plugin settings in a Maya option var.
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


def fillUI(jobContext, settings, container, rowLayout):
    '''
    Exporter plugin UI creation and filling function, receving a Qt container to fill.
    You can create a different layout for the container if you wish to replace the
    default row layout.
    '''
    # We want to use the latest stride value passed in, in case the user disabled our plugin,
    # modified the value, re-enabled our plugin and now want to editthe value. For many plugins,
    # all edited value will be plugin-specific, so this is not necessary since the UI we are
    # about to create could be assumed to be the only way to edit the value.
    stride = settings[strideToken]

    strideEdit = QtW.QLineEdit(str(stride))
    strideEdit.setValidator(QtG.QDoubleValidator())
    strideEdit.setObjectName(strideToken)
    
    rowLayout.addWidget(QtW.QLabel(tokenToLabel(strideToken)))
    rowLayout.addWidget(strideEdit)


def queryUI(jobContext, settings, container):
    '''
    Exporter plugin UI query to retrieve the data when teh UI is confirmed by the user.
    '''
    strideEdit = container.findChild(QtW.QLineEdit, strideToken)
    if strideEdit:
        global exampleSettings
        stride = float(strideEdit.text())
        exampleSettings[strideToken] = stride
        settings[strideToken] = stride


def showUi(jobContext, parentUIName, settings):
    '''
    Generic function to show a dialog with OK/cancel buttons.
    '''
    try:
        parent = mui.MQtUtil.findControl(parentUIName)
        parentWidget = wrapInstance(int(parent), QtW.QWidget)
        while not parentWidget.isWindow():
            parentWidget = parentWidget.parent()

        window = QtW.QDialog(parentWidget)
        window.setModal(True)
        window.setWindowTitle("Options for %s" % jobContext)
        
        rowLayout = QtW.QVBoxLayout()
        window.setLayout(rowLayout)

        container = QtW.QWidget()
        rowLayout.addWidget(container)

        buttonBox = QtW.QDialogButtonBox(QtW.QDialogButtonBox.Ok | QtW.QDialogButtonBox.Cancel)
        rowLayout.addWidget(buttonBox)

        def accept():
            queryUI(jobContext, settings, container)
            saveSettings()
            window.accept()
        
        buttonBox.accepted.connect(accept)
        buttonBox.rejected.connect(window.reject)
        
        rowLayout = QtW.QVBoxLayout()
        container.setLayout(rowLayout)
        fillUI(jobContext, settings, container, rowLayout)

        window.exec()
    except Exception as ex:
        logDebug("Error: " + str(ex))
    except:
        logDebug("Error")
    

def exportUiFn(jobContext, parentUIName, settings):
    '''
    The exporter plugin UI callback, shows a UI to edit the settings it want forced.
    '''
    logDebug("%s export plugin UI called for job context %s in %s with settings %s" %
          (exampleJobContextName, jobContext, parentUIName, str(settings)))
    
    showUi(jobContext, parentUIName, settings)

    logDebug("%s export plugin UI returning: %s" %
          (exampleJobContextName, settings))

    return settings


def exportEnablerFn():
    '''
    The exporter plugin settings callback, returns the settings it want forced with the forced value.
    '''
    global exampleSettings
    logDebug("%s export plugin settings called, returning: %s" %
          (exampleJobContextName, exampleSettings))
    return exampleSettings


def register():
    '''
    Register the exporter plugin with an exporter settings callback and a UI callback.
    '''
    logDebug("Registering the %s export plugin" % exampleJobContextName)
    mayaUsd.lib.JobContextRegistry.RegisterExportJobContext(
        exampleJobContextName, exampleFriendlyName, exampleDescription, exportEnablerFn)

    mayaUsd.lib.JobContextRegistry.SetExportOptionsUI(
        exampleJobContextName, exportUiFn)
    
    loadSettings()


register()

