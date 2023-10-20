import maya.cmds as cmds
import maya.mel as mel
import maya.OpenMayaUI as omui
import mayaUsd.lib as mayaUsdLib
from mayaUSDRegisterStrings import getMayaUsdString
import mayaUsdMayaReferenceUtils as mayaRefUtils
import mayaUsdUtils

try:
    from PySide2.QtWidgets import QFileDialog, QLineEdit, QDialogButtonBox, QComboBox, QApplication
except:
    from PySide6.QtWidgets import QFileDialog, QLineEdit, QDialogButtonBox, QComboBox, QApplication

# Global variables
_relativeToFilePath = None

class usdFileRelative(object):
    '''
    Helper class to create the UI for load/save dialog boxes that need to make the
    selected file name optionally relative.

    The caller must tell each function to what the file should be made relative to.
    The 'what' is used to select the correct UI element, UI label, UI tool-tip and
    to read and write the correct option var.

    For example by passing 'SceneFile', the functions will use:
        UI element:           MakePathRelativeToSceneFile
        UI label:            kMakePathRelativeToSceneFile
        UI tool-tip:      kMakePathRelativeToSceneFileAnn
        option var:   mayaUsd_MakePathRelativeToSceneFile
    '''
    # We will find (and store) some of the dialog controls so we can query them.
    # Note: are initialized when calling the uiInit() method.
    _fileDialog = None
    _acceptButton = None

    kMakePathRelativeCheckBox = 'MakePathRelativeTo'
    kUnresolvedPathTextField = 'UnresolvedPath'

    # We will store whether or not the file can be made relative. Used to know when we need
    # to update fields.
    # Note: are initialized in the uiInit() method.
    _canBeRelative = False
    _haveRelativePathFields = False
    _relativeToDir = ""
    _relativeToScene = False
    _ensureUsdExtension = True
    _checkBoxClass = None

    @staticmethod
    def setRelativeFilePathRoot(filePath):
        '''
        Sets to which file the relative file will be anchored.
        Set to empty or None to have the file be absolute, not relative.
        '''
        global _relativeToFilePath
        _relativeToFilePath = filePath

    @staticmethod
    def getRelativeFilePathRoot():
        '''
        Gets to which file the relative file will be anchored.
        Empty or None to have the file be absolute, not relative.
        '''
        global _relativeToFilePath
        return _relativeToFilePath
    
    @classmethod
    def uiCreate(cls, parentLayout):
        """
        Helper method to create the UI layout for the file relative actions.

        Input parentLayout arg is expected to be a scroll layout into which controls
        can be added.
        """
        mayaRefUtils.pushOptionsUITemplate()
        cmds.setParent(parentLayout)

        topForm = cmds.columnLayout('actionOptionsForm', rowSpacing=5)

        kRelativePathOptionsStr = getMayaUsdString("kRelativePathOptions")
 
        cmds.setParent(topForm)
        cmds.frameLayout(label=kRelativePathOptionsStr, collapsable=False)
        widgetColumn = cmds.columnLayout()

        cls.uiCreateFields()

        return topForm

    @classmethod
    def uiCreateFields(cls, useCheckBoxGrp=False):
        """
        Helper method to create the UI fields for the file relative actions.
        """
        cls._checkBoxClass = CheckboxGroup if useCheckBoxGrp else Checkbox

        kMakePathRelativeStr = getMayaUsdString("kMakePathRelativeTo" + cls.kRelativeToWhat)
        kMakePathRelativeAnnStr = getMayaUsdString("kMakePathRelativeTo" + cls.kRelativeToWhat + "Ann")
        kUnresolvedPathStr = getMayaUsdString("kUnresolvedPath")
        kUnresolvedPathAnnStr = getMayaUsdString("kUnresolvedPathAnn")

        cls._checkBoxClass.create(cls.kMakePathRelativeCheckBox, kMakePathRelativeStr, kMakePathRelativeAnnStr)
        cls._checkBoxClass.command(cls.kMakePathRelativeCheckBox, cls.onMakePathRelativeChanged)
        cmds.textFieldGrp(cls.kUnresolvedPathTextField, label=kUnresolvedPathStr, ann=kUnresolvedPathAnnStr, editable=False)
        cls._haveRelativePathFields = True

    @classmethod
    def uiInit(cls, parentLayout):
        """
        Helper method to initialize the UI layout for the file relative actions.

        Input parentLayout arg is expected to be a scroll layout into which controls
        can be added.
        """
        cmds.setParent(parentLayout)

        # Initialize the class variables.
        cls._fileDialog = None
        cls._acceptButton = None
        cls._ensureUsdExtension = True
        cls._haveRelativePathFields = cmds.textFieldGrp(cls.kUnresolvedPathTextField, exists=True)

        # Get the current checkbox value from optionVar (if any) and update checkbox.
        if cmds.optionVar(exists='mayaUsd_MakePathRelativeTo' + cls.kRelativeToWhat):
            relative = cmds.optionVar(query='mayaUsd_MakePathRelativeTo' + cls.kRelativeToWhat)
            cls._checkBoxClass.set(cls.kMakePathRelativeCheckBox, relative)

        # If if cannot be relative, then the checkbox and label should be disabled.
        cls._checkBoxClass.enable(cls.kMakePathRelativeCheckBox, cls._canBeRelative)

        # We only need to connect to the dialog box controls when we can be relative because
        # that is when the file path preview fields are enabled.
        if cls._canBeRelative:
            cls.connectToDialogControls(parentLayout)

        if cls._haveRelativePathFields:
            # We may need to hide the preview fields in certain cases
            showPreviewFields = True
            if cls.kRelativeToWhat == 'SceneFile':
                showPreviewFields = cmds.file(q=True, exists=True)
            else:
                showPreviewFields = bool(cls._relativeToDir)

            cmds.textFieldGrp(cls.kUnresolvedPathTextField, edit=True, visible=showPreviewFields)

            # Only enable fields when make relative is checked.
            makePathRelative = cls._checkBoxClass.get(cls.kMakePathRelativeCheckBox)
            cls.onMakePathRelativeChanged(makePathRelative)

    @classmethod
    def uiCommit(cls, parentLayout):
        """
        Helper method to commit the UI layout for the file relative actions.

        Input parentLayout arg is expected to the a scroll layout into which controls
        can be added.
        """
        cmds.setParent(parentLayout)

        # Get the current checkbox state and save to optionVar.
        relative = cls._checkBoxClass.get(cls.kMakePathRelativeCheckBox)
        cmds.optionVar(iv=('mayaUsd_MakePathRelativeTo' + cls.kRelativeToWhat, relative))

    @staticmethod
    def findWindowNameFromLayout(layoutName):
        """
        Find the window name that contains the given layout.
        """
        window_name = cmds.layout(layoutName, query=True, parent=True)
        if '|' in window_name:
            window_name = window_name.split('|')[0]
        return cmds.window(window_name, query=True, title=True)

    @staticmethod
    def findQtWindowFromTitle(title):
        """
        Find the Qt window that has the given title.
        """
        for window in QApplication.topLevelWidgets():
            if window.windowTitle() == title:
                return window
        return None

    @classmethod
    def connectToDialogControls(cls, parentLayout):
        """
        Find some of the conrols (such as file name edit field) in the dialog
        (under the input parent layout) and connect to them for change notifs.
        Used so we can update the file path preview fields.
        """

        # Get the Qt Window containing the input parent layout from mel.
        maya_window_name = usdFileRelative.findWindowNameFromLayout(parentLayout)
        maya_window = usdFileRelative.findQtWindowFromTitle(maya_window_name)
        if maya_window:
            cls._fileDialog = maya_window
            # Find the file name edit field and connect to it to be notified when text changes.
            fileNameEditField = maya_window.findChild(QLineEdit, 'fileNameEdit')
            if fileNameEditField:
                fileNameEditField.textChanged.connect(cls.onfileNameEditFieldChanged)

            # Find the accept button, so we can know when there is valid input in the file name edit field.
            buttonBox = maya_window.findChild(QDialogButtonBox, 'buttonBox')
            if buttonBox and cls._fileDialog:
                btn = QDialogButtonBox.Open if (cls._fileDialog.acceptMode() == QFileDialog.AcceptOpen) else QDialogButtonBox.Save
                cls._acceptButton = buttonBox.button(btn)

            # Find the lookin combobox and connect to it so we know when user changes directories.
            lookinComboBox = maya_window.findChild(QComboBox, 'lookInCombo')
            if lookinComboBox:
                lookinComboBox.currentTextChanged.connect(cls.onLookinTextChanged)

    @classmethod
    def onMakePathRelativeChanged(cls, checked):
        """Callback from MakePathRelativeCheckBox changeCommand."""
        if not cls._haveRelativePathFields:
            return
        enableFields = cls._canBeRelative and checked
        cmds.textFieldGrp(cls.kUnresolvedPathTextField, edit=True, enable=enableFields, text='')
        if enableFields:
            cls.updateFilePathPreviewFields(cls.getRawSelectedFile(), checked)

    @classmethod
    def onfileNameEditFieldChanged(cls, text):
        """Callback from Qt textChanged event from fileNameEditField."""
        cls.updateFilePathRelatedUi(cls.getRawSelectedFile())

    @classmethod
    def onLookinTextChanged(cls, text):
        """Callback from Qt currentTextChanged from LookIn combobox."""
        cls.updateFilePathRelatedUi(cls.getRawSelectedFile())

    @classmethod
    def getRawSelectedFile(cls):
        """Determine what the currently-selected file full path name."""
        # If the accept button is disabled it means there is no valid input in the file
        # name edit field.
        selFiles = cls._fileDialog.selectedFiles() if cls._fileDialog and cls._acceptButton and cls._acceptButton.isEnabled() else None
        selectedFile = selFiles[0] if selFiles else ''

        if cls._ensureUsdExtension:
            # Make sure the file path has a valid USD extension. This is NOT done by the fileDialog so
            # the user is free to enter any extension they want. The layer editor code will then verify
            # (and fix if needed) the file path before saving. We do the same here for preview.
            selectedFile = mayaUsdLib.Util.ensureUSDFileExtension(selectedFile) if selectedFile else ''

        return selectedFile

    @classmethod
    def updateFilePathRelatedUi(cls, unresolvedPath):
        """Update all UI that cares about the given selected file."""
        cls.updateFilePathPreviewFields(unresolvedPath)

    @classmethod
    def updateFilePathPreviewFields(cls, unresolvedPath, makePathRelative=None):
        """Update the file-path preview UI."""
        if not cls._haveRelativePathFields:
            return

        if not cls._canBeRelative:
            return

        if makePathRelative == None:
            makePathRelative = cls._checkBoxClass.get(cls.kMakePathRelativeCheckBox)

        if not makePathRelative:
            return

        relativePath = ''
        if unresolvedPath and cls._relativeToDir:
            relativePath = mayaUsdLib.Util.getPathRelativeToDirectory(unresolvedPath, cls._relativeToDir)
        elif unresolvedPath and cls._relativeToScene:
            relativePath = mayaUsdLib.Util.getPathRelativeToMayaSceneFile(unresolvedPath)
        cmds.textFieldGrp(cls.kUnresolvedPathTextField, edit=True, text=relativePath)

    @classmethod
    def selectionChanged(cls, parentLayout, selection):
        """Callback from fileDialog selectionChanged."""
        cmds.setParent(parentLayout)
        cls.updateFilePathRelatedUi(cls.getRawSelectedFile())

    @classmethod
    def fileTypeChanged(cls, parentLayout, newType):
        """Callback from fileDialog command fileTypeChanged."""
        cmds.setParent(parentLayout)
        cls.updateFilePathRelatedUi(cls.getRawSelectedFile())

class usdRootFileRelative(usdFileRelative):
    '''
    Helper class to create the UI for load/save dialog boxes that need to make the
    selected file name optionally relative to the Maya scene file.
    '''

    kRelativeToWhat = 'SceneFile'

    @classmethod
    def uiCreate(cls, parentLayout):
        return super(usdRootFileRelative, cls).uiCreate(parentLayout)

    @classmethod
    def uiInit(cls, parentLayout, filterType):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
        # USD root layer can always be set relative to Maya scene file. 
        # If the latter doesn't exist, we use a special approach with 
        # postponed relative file path assignment 
        cls.setRelativeFilePathRoot(cmds.file(query=True, sceneName=True))
        cls._relativeToScene = True
        cls._canBeRelative = True
        super(usdRootFileRelative, cls).uiInit(parentLayout)

    @classmethod
    def uiCommit(cls, parentLayout, selectedFile=None):
        '''
        Note: the function takes an unused selectedFile argument to be compatible
              with the dialog2 command API.
        '''
        super(usdRootFileRelative, cls).uiCommit(parentLayout)


class usdSubLayerFileRelative(usdFileRelative):
    '''
    Helper class to create the UI for load/save dialog boxes that need to make the
    selected file name optionally relative to a parent layer.
    '''

    kRelativeToWhat = 'ParentLayer'

    @classmethod
    def uiCreate(cls, parentLayout):
        return super(usdSubLayerFileRelative, cls).uiCreate(parentLayout)

    @classmethod
    def uiInit(cls, parentLayout, filterType, parentLayerPath):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
        cls.setRelativeFilePathRoot(parentLayerPath)
        cls._relativeToDir = parentLayerPath
        # Even if the parent layer is not saved, the file can still be marked as relative.
        # In that case we use a technique to set the path to relative in a postponed fashion.
        cls._canBeRelative = True
        super(usdSubLayerFileRelative, cls).uiInit(parentLayout)

    @classmethod
    def uiCommit(cls, parentLayout, selectedFile=None):
        '''
        Note: the function takes an unused selectedFile argument to be compatible
              with the dialog2 command API.
        '''
        super(usdSubLayerFileRelative, cls).uiCommit(parentLayout)


class usdFileRelativeToEditTargetLayer(usdFileRelative):
    '''
    Helper class to create the UI for load/save dialog boxes that need to make the
    selected file name optionally relative to a layer file.
    '''

    kRelativeToWhat = 'EditTargetLayer'

    @classmethod
    def uiCreate(cls, parentLayout):
        return super(usdFileRelativeToEditTargetLayer, cls).uiCreate(parentLayout)

    @classmethod
    def uiInit(cls, parentLayout, filterType):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
        cls._relativeToDir = usdFileRelative.getRelativeFilePathRoot()
        # Even if the target layer is not saved, the file can still be marked as relative.
        # In that case we use a technique to set the path to relative in a postponed fashion.
        cls._canBeRelative = True
        super(usdFileRelativeToEditTargetLayer, cls).uiInit(parentLayout)

    @classmethod
    def uiCommit(cls, parentLayout, selectedFile=None):
        '''
        Note: the function takes an unused selectedFile argument to be compatible
              with the dialog2 command API.
        '''
        super(usdFileRelativeToEditTargetLayer, cls).uiCommit(parentLayout)

class usdAddRefOrPayloadRelativeToEditTargetLayer(usdFileRelativeToEditTargetLayer):
    '''
    Helper class to create the UI for add reference or payload optionally
    relative to a layer file.
    '''

    @classmethod
    def uiCreate(cls, parentLayout):
        topForm = super(usdAddRefOrPayloadRelativeToEditTargetLayer, cls).uiCreate(parentLayout)

        cmds.setParent(topForm)
        cmds.frameLayout(label=getMayaUsdString("kCompositionArcOptions"), collapsable=False)

        mayaRefUtils.createUsdRefOrPayloadUI(True)

        return topForm

    @classmethod
    def uiInit(cls, parentLayout, filterType):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
        super(usdAddRefOrPayloadRelativeToEditTargetLayer, cls).uiInit(parentLayout, filterType)

        wantRef = mayaUsdUtils.wantReferenceCompositionArc()
        wantPrepend = mayaUsdUtils.wantPrependCompositionArc()
        wantLoad = mayaUsdUtils.wantPayloadLoaded()

        compositionArc = mayaRefUtils.compositionArcReference if wantRef else mayaRefUtils.compositionArcPayload
        listEditType = mayaRefUtils.listEditTypePrepend if wantPrepend else mayaRefUtils.listEditTypeAppend
        loadPayload = bool(wantLoad)

        values = {
            mayaRefUtils.compositionArcKey: compositionArc,
            mayaRefUtils.listEditTypeKey: listEditType,
            mayaRefUtils.loadPayloadKey: loadPayload,
        }
        mayaRefUtils.initUsdRefOrPayloadUI(values, True)

    @classmethod
    def uiCommit(cls, parentLayout, selectedFile=None):
        '''
        Note: the function takes an unused selectedFile argument to be compatible
              with the dialog2 command API.
        '''
        super(usdAddRefOrPayloadRelativeToEditTargetLayer, cls).uiCommit(parentLayout, selectedFile)

        values = mayaRefUtils.commitUsdRefOrPayloadUI(True)

        compositionArc = values[mayaRefUtils.compositionArcKey]
        listEditType = values[mayaRefUtils.listEditTypeKey]
        loadPayload = values[mayaRefUtils.loadPayloadKey]
        primPath = values[mayaRefUtils.referencedPrimPathKey]

        wantReference = bool(compositionArc == mayaRefUtils.compositionArcReference)
        wantPrepend = bool(listEditType == mayaRefUtils.listEditTypePrepend)
        wantLoad = bool(loadPayload)

        mayaUsdUtils.saveWantReferenceCompositionArc(wantReference)
        mayaUsdUtils.saveWantPrependCompositionArc(wantPrepend)
        mayaUsdUtils.saveWantPayloadLoaded(wantLoad)
        mayaUsdUtils.saveReferencedPrimPath(primPath)

    @classmethod
    def updateFilePathRelatedUi(cls, unresolvedPath):
        """Update all UI that cares about the given selected file."""
        super(usdAddRefOrPayloadRelativeToEditTargetLayer, cls).updateFilePathRelatedUi(unresolvedPath)
        cls.updateMayaReferenceUi(unresolvedPath)

    @classmethod
    def updateMayaReferenceUi(cls, unresolvedPath):
        mayaRefUtils.updateUsdRefOrPayloadUI(unresolvedPath)


class usdImageRelativeToEditTargetLayer(usdFileRelativeToEditTargetLayer):
    '''
    Helper class to create the UI for image optionally relative to a layer file.
    '''

    kRelativeToWhat = 'ImageEditTargetLayer'

    @classmethod
    def uiInit(cls, parentLayout, filterType):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
        super(usdImageRelativeToEditTargetLayer, cls).uiInit(parentLayout, filterType)

        # Do not force the USD file extension since we are selecting image files.
        cls._ensureUsdExtension = False


class usdMayaRefRelativeToEditTargetLayer(usdFileRelativeToEditTargetLayer):
    '''
    Helper class to create the UI for Maya ref optionally relative to a layer file
    via the current USD edit target.
    '''
    @classmethod
    def uiInit(cls, parentLayout, filterType):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
        super(usdMayaRefRelativeToEditTargetLayer, cls).uiInit(parentLayout, filterType)

        # Do not force the USD file extension since we are selecting Maya files.
        cls._ensureUsdExtension = False

# Abstract away the difference between check-box and check-box group

class Checkbox:
    @staticmethod
    def create(uiName, label, tooltip):
        cmds.checkBox(uiName, label=label, ann=tooltip)

    @staticmethod
    def command(uiName, cmd):
        cmds.checkBox(uiName, edit=True, changeCommand=cmd)

    @staticmethod
    def set(uiName, value):
        cmds.checkBox(uiName, edit=True, value=value)

    @staticmethod
    def enable(uiName, enabled):
        cmds.checkBox(uiName, edit=True, enable=enabled)

    @staticmethod
    def get(uiName):
        return cmds.checkBox(uiName, query=True, value=True)

class CheckboxGroup:
    @staticmethod
    def create(uiName, label, tooltip):
        cmds.checkBoxGrp(uiName, numberOfCheckBoxes=1, label="",
                        label1=label, ann=tooltip)

    @staticmethod
    def command(uiName, cmd):
        cmds.checkBoxGrp(uiName, edit=True, cc1=cmd)

    @staticmethod
    def set(uiName, value):
        cmds.checkBoxGrp(uiName, edit=True, v1=value)

    @staticmethod
    def enable(uiName, enabled):
        cmds.checkBoxGrp(uiName, edit=True, enable=enabled)

    @staticmethod
    def get(uiName):
        return cmds.checkBoxGrp(uiName, query=True, v1=True)
