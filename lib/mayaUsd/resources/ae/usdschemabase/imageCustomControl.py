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

from .attributeCustomControl import AttributeCustomControl
from .attributeCustomControl import cleanAndFormatTooltip

import mayaUsd.lib as mayaUsdLib
from mayaUsdLibRegisterStrings import getMayaUsdLibString
import mayaUsd_USDRootFileRelative as murel
import mayaUsdUtils

import maya.cmds as cmds
import maya.mel as mel
import maya.internal.ufeSupport.attributes as attributes

from maya.common.ui import LayoutManager, ParentManager

import ufe

import os
from pxr import Sdf, UsdShade

try:
    from PySide2 import QtCore
except:
    from PySide6 import QtCore


class ImageCustomControl(AttributeCustomControl):
    filenameField = "UIFilenameField"

    @staticmethod
    def isImageAttribute(aeTemplate, attrName):
        '''
        Verify if the given attribute name is an image attribute for the prim
        currently being shown in the given AE template.
        '''
        kFilenameAttr = ufe.Attribute.kFilename if hasattr(ufe.Attribute, "kFilename") else 'Filename'
        if aeTemplate.attrs.attributeType(attrName) != kFilenameAttr:
            return False
        shader = UsdShade.Shader(aeTemplate.prim)
        if shader and attrName.startswith("inputs:"):
            # Shader attribute. The actual USD Attribute might not exist yet.
            return True
        attr = aeTemplate.prim.GetAttribute(attrName)
        if not attr:
            return False
        typeName = attr.GetTypeName()
        if not typeName:
            return False
        return aeTemplate.assetPathType == typeName.type

    @staticmethod
    def creator(aeTemplate, attrName, label=None):
        if not ImageCustomControl.isImageAttribute(aeTemplate, attrName):
            return None
        ufeAttr = aeTemplate.attrs.attribute(attrName)
        return ImageCustomControl(ufeAttr, aeTemplate.prim, attrName, aeTemplate.useNiceName, label=label)

    def __init__(self, ufeAttr, prim, attrName, useNiceName, label=None):
        super(ImageCustomControl, self).__init__(ufeAttr, attrName, useNiceName, label=label)
        self.prim = prim
        self.controlName = None

    def onCreate(self, *args):
        cmds.setUITemplate("attributeEditorTemplate", pst=True)

        attrUIName = self.getUILabel()
        ufeAttr = self.ufeAttr

        createdControl = cmds.rowLayout(nc=3)
        self.controlName = createdControl
        with LayoutManager(createdControl):
            cmds.text(label=attrUIName, annotation=cleanAndFormatTooltip(ufeAttr.getDocumentation()))
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

        # Determine if we can get an absolute path from the attribute value.
        # If we can then try and find the starting directory from that path.
        startDir = None
        usdAttr = self.prim.GetAttribute(self.attrName)
        usdValue = usdAttr.Get()
        if isinstance(usdValue, Sdf.AssetPath):
            resolvedPath = usdValue.resolvedPath
            if os.path.exists(resolvedPath):
                # Use forward slashes in path to avoid any problems. Maya accepts them all platforms.
                startDir = os.path.abspath(os.path.dirname(resolvedPath)).replace(os.sep, '/')

        dialogArgs = {
            'startDir'          : startDir if startDir else mel.eval("""setWorkingDirectory("%s", "image", "sourceImages")""" % workspace),
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
            fileName = mayaUsdLib.Util.handleAssetPathThatMaybeRelativeToLayer(  \
                        files[0],                                                \
                        self.ufeAttr.name,                                       \
                        self.prim.GetStage().GetEditTarget().GetLayer(),         \
                        "mayaUsd_MakePathRelativeToImageEditTargetLayer")
            callback(ImageCustomControl.fromNativePath(fileName), "")

    @staticmethod
    def prepareRelativeDir(prim):
        layerDirName = mayaUsdUtils.getCurrentTargetLayerDir(prim)
        murel.usdFileRelative.setRelativeFilePathRoot(layerDirName)


