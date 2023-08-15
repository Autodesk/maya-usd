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

import mayaUsd.lib as mayaUsdLib
from mayaUsdLibRegisterStrings import getMayaUsdLibString
import mayaUsd_USDRootFileRelative as murel
import mayaUsdUtils

import maya.cmds as cmds
import maya.mel as mel
import maya.internal.ufeSupport.attributes as attributes

from maya.common.ui import LayoutManager, ParentManager

import ufe

import os.path

try:
    from PySide2 import QtCore
except:
    from PySide6 import QtCore


class ImageCustomControl(object):
    filenameField = "UIFilenameField"

    def __init__(self, ufeAttr, prim, attrName, useNiceName):
        self.ufeAttr = ufeAttr
        self.prim = prim
        self.attrName = attrName
        self.useNiceName = useNiceName
        self.controlName = None
        super(ImageCustomControl, self).__init__()

    def onCreate(self, *args):
        cmds.setUITemplate("attributeEditorTemplate", pst=True)

        attrUIName = mayaUsdLib.Util.prettifyName(self.attrName) if self.useNiceName else self.attrName
        ufeAttr = self.ufeAttr

        createdControl = cmds.rowLayout(nc=3)
        self.controlName = createdControl
        with LayoutManager(createdControl):
            cmds.text(label=attrUIName)
            cmds.textField(ImageCustomControl.filenameField)
            cmds.symbolButton("browser", image="navButtonBrowse.png")

        cmds.setUITemplate(ppt=True)
        pMenu = attributes.AEPopupMenu(createdControl, ufeAttr)

        self.onReplace()
        return createdControl

    def onReplace(self, *args):
        ufeAttr = self.ufeAttr
        controlName = self.controlName
        with ParentManager(controlName):
            self.updateUi(ufeAttr, controlName)
            callback = lambda filename,filetype : self.assignFilename(ufeAttr, filename, filetype)
            command = lambda *_a : self.filenameBrowser(callback)
            cmds.button("browser", c=command, e=True)

        self.attachCallbacks(ufeAttr, controlName, None)

    def updateUi(self, attr, uiControlName):
        '''Callback function to update the UI when the data model changes.'''
        isLocked = attributes.isAttributeLocked(attr)
        bgClr = attributes.getAttributeColorRGB(attr)
        # We might get called from the text field instead of the row layout:
        fieldPos = uiControlName.find(ImageCustomControl.filenameField)
        if fieldPos > 0:
            uiControlName = uiControlName[:fieldPos - 1]
        with ParentManager(uiControlName):
            cmds.textField(ImageCustomControl.filenameField, e=True, tx=attr.get(), editable=not isLocked)
            if bgClr:
                cmds.textField(ImageCustomControl.filenameField, e=True, backgroundColor=bgClr)

    def attachCallbacks(self, ufeAttr, uiControl, changedCommand):
        # Create change callback for UFE and UI value synchronization.
        filenameControl = uiControl + "|" + ImageCustomControl.filenameField
        uiControlDesc = attributes.UIControlDescriptor(filenameControl, 'textField', hasDrag=False)
        cb = attributes.createChangeCb(self.updateUi, ufeAttr, uiControlDesc)
        with ParentManager(uiControl):
            cmds.textField(ImageCustomControl.filenameField, edit=True, changeCommand=cb)

    def assignFilename(self, ufeAttr, filename, fileType):
        ufeFullPath = '%s.%s' % (ufe.PathString.string(ufeAttr.sceneItem().path()), ufeAttr.name)
        cmds.setAttr(ufeFullPath, filename)
        currentDir = cmds.workspace(q=True, dir=True)
        mel.eval("""retainWorkingDirectory("%s")""" % currentDir)
        self.updateUi(ufeAttr, self.controlName)
        return True

    @staticmethod
    def fromNativePath(path):
        if cmds.about(nt=True):
            return cmds.encodeString(path).replace("\\\\", "/")
        return path
    
    @staticmethod
    def fixFileDialogSplitters():
        org = QtCore.QCoreApplication.organizationName()
        app = QtCore.QCoreApplication.applicationName()
        settings = QtCore.QSettings(org, app)
        kFileDialogSettingsGroup = 'FileDialog'
        kOptionsSplittersKey = 'optionsSplitters'
        settings.beginGroup(kFileDialogSettingsGroup)
        size = settings.beginReadArray(kOptionsSplittersKey)
        if size < 2 or settings.value(str(1), 0) <= 0:
            settings.endArray()
            settings.beginWriteArray(kOptionsSplittersKey)
            for i in range(2):
                settings.setArrayIndex(i)
                settings.setValue(str(i), 120 + 120 * i)
            settings.endArray()
        settings.endArray()
        settings.endGroup()

    def filenameBrowser(self, callback):
        workspace = cmds.workspace(q=True, fn=True)

        ImageCustomControl.fixFileDialogSplitters()

        dialogArgs = {
            'startDir'          : mel.eval("""setWorkingDirectory("%s", "image", "sourceImages")""" % workspace),
            'filter'            : mel.eval("buildImageFileFilterList()"),
            'caption'           : getMayaUsdLibString('kOpenImage'),
            'createCallback'    : "mayaUsd_ImageFileRelative_UICreate",
            'initCallback'      : "mayaUsd_ImageFileRelative_UIInit",
            'commitCallback'    : "mayaUsd_ImageFileRelative_UICommit",
            'fileTypeCallback'  : "mayaUsd_ImageFileRelative_FileTypeChanged",
            'selectionCallback' : "mayaUsd_ImageFileRelative_SelectionChanged",
        }

        cmd = r'''fileDialog2''' \
              r''' -caption "{caption}"''' \
              r''' -fileMode 1''' \
              r''' -fileFilter "{filter}"''' \
              r''' -selectFileFilter "All Files"''' \
              r''' -startingDirectory "{startDir}"''' \
              r''' -optionsUICreate {createCallback}''' \
              r''' -optionsUIInit {initCallback}''' \
              r''' -optionsUICommit2 {commitCallback}''' \
              r''' -fileTypeChanged {fileTypeCallback}''' \
              r''' -selectionChanged {selectionCallback}'''.format(**dialogArgs)
        
        ImageCustomControl.prepareRelativeDir(self.prim)

        files = mel.eval(cmd)

        if files != None and len(files) > 0:
            fileName = files[0]
            if ImageCustomControl.wantRelativeFileName():
                layerDirName = ImageCustomControl.getCurrentTargetLayerDir(self.prim)
                fileName = mayaUsdLib.Util.getPathRelativeToDirectory(fileName, layerDirName)
            callback(ImageCustomControl.fromNativePath(fileName), "")

    @staticmethod
    def wantRelativeFileName():
        opVarName = "mayaUsd_MakePathRelativeToImageEditTargetLayer"
        return cmds.optionVar(exists=opVarName) and cmds.optionVar(query=opVarName)

    @staticmethod
    def prepareRelativeDir(prim):
        layerDirName = mayaUsdUtils.getCurrentTargetLayerDir(prim)
        murel.usdFileRelative.setRelativeFilePathRoot(layerDirName)


def customImageControlCreator(aeTemplate, c):
    if not aeTemplate.isImageAttribute(c):
        return None
    ufeAttr = aeTemplate.attrS.attribute(c)
    return ImageCustomControl(ufeAttr, aeTemplate.prim, c, aeTemplate.useNiceName)

