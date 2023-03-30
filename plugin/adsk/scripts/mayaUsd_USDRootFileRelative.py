import maya.cmds as cmds
import maya.mel as mel
import maya.OpenMayaUI as omui
import mayaUsd.lib as mayaUsdLib
from mayaUSDRegisterStrings import getMayaUsdString
from mayaUsdMayaReferenceUtils import pushOptionsUITemplate

try:
    from PySide2.QtCore import QObject, QFileInfo
    from PySide2.QtWidgets import QFileDialog, QLayout, QWidget, QLineEdit, QDialogButtonBox, QComboBox
    from shiboken2 import wrapInstance
except:
    from PySide6.QtCore import QObject, QFileInfo
    from PySide6.QtWidgets import QFileDialog, QLayout, QWidget, QLineEdit, QDialogButtonBox, QComboBox
    from shiboken6 import wrapInstance

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
    kResolvedPathTextField = 'ResolvedPath'

    # We will store whether or not the file can be made relative. Used to know when we need
    # to update fields.
    # Note: are initialized in the uiInit() method.
    _canBeRelative = False
    _haveRelativePathFields = False
    _parentLayerPath = ""

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
    def uiCreate(cls, parentLayout, relativeToWhat):
        """
        Helper method to create the UI layout for the file relative actions.

        Input parentLayout arg is expected to the a scroll layout into which controls
        can be added.

        Input relativeToWhat tells what the file is relative to. See the class docs.
        """
        pushOptionsUITemplate()
        cmds.setParent(parentLayout)

        optBoxForm = cmds.formLayout('optionsBoxForm')
        topFrame = cmds.frameLayout(
            'optionsBoxFrame', collapsable=False, labelVisible=False,
            marginWidth=10, borderVisible=False)
        cmds.formLayout(optBoxForm, edit=True,
                        af=[(topFrame, 'left', 0),
                            (topFrame, 'top', 0),
                            (topFrame, 'right', 0),
                            (topFrame, 'bottom', 0)])
        
        topForm = cmds.columnLayout('actionOptionsForm', rowSpacing=5)

        kFileOptionsStr = getMayaUsdString("kFileOptions")
        kMakePathRelativeStr = getMayaUsdString("kMakePathRelativeTo" + relativeToWhat)
        kMakePathRelativeAnnStr = getMayaUsdString("kMakePathRelativeTo" + relativeToWhat + "Ann")
        kUnresolvedPathStr = getMayaUsdString("kUnresolvedPath")
        kUnresolvedPathAnnStr = getMayaUsdString("kUnresolvedPathAnn")
        kResolvedPathStr = getMayaUsdString("kResolvedPath")
        kResolvedPathAnnStr = getMayaUsdString("kResolvedPathAnn")
 
        optBoxMarginWidth = mel.eval('global int $gOptionBoxTemplateDescriptionMarginWidth; $gOptionBoxTemplateDescriptionMarginWidth += 0')
        cmds.setParent(topForm)
        cmds.frameLayout(label=kFileOptionsStr, collapsable=False)
        widgetColumn = cmds.columnLayout()
        cmds.checkBox(cls.kMakePathRelativeCheckBox, label=kMakePathRelativeStr, ann=kMakePathRelativeAnnStr)
        
        cmds.checkBox(cls.kMakePathRelativeCheckBox, edit=True, changeCommand=cls.onMakePathRelativeChanged)
        cmds.textFieldGrp(cls.kUnresolvedPathTextField, label=kUnresolvedPathStr, ann=kUnresolvedPathAnnStr, editable=False)
        cmds.textFieldGrp(cls.kResolvedPathTextField, label=kResolvedPathStr, ann=kResolvedPathAnnStr , editable=False)
        cls._haveRelativePathFields = True

    @classmethod
    def uiInit(cls, parentLayout, canBeRelative, relativeToWhat):
        """
        Helper method to initialize the UI layout for the file relative actions.

        Input parentLayout arg is expected to be a scroll layout into which controls
        can be added.

        Input canBeRelative tells if the file can be made relative at all. If false,
        the relative path UI is shown but disabled.

        Input relativeToWhat tells what the file is relative to. See the class docs.
        """
        cmds.setParent(parentLayout)

        # Initialize the class variables.
        cls._fileDialog = None
        cls._acceptButton = None
        cls._canBeRelative = canBeRelative
        cls._haveRelativePathFields = cmds.textFieldGrp(cls.kUnresolvedPathTextField, exists=True)

        # Get the current checkbox value from optionVar (if any) and update checkbox.
        if cmds.optionVar(exists='mayaUsd_MakePathRelativeTo' + relativeToWhat):
            relative = cmds.optionVar(query='mayaUsd_MakePathRelativeTo' + relativeToWhat)
            cmds.checkBox(cls.kMakePathRelativeCheckBox, edit=True, value=relative)

        # If if cannot be relative, then the checkbox and label should be disabled.
        cmds.checkBox(cls.kMakePathRelativeCheckBox, edit=True, enable=cls._canBeRelative)

        # We only need to connect to the dialog box controls when we can be relative because
        # that is when the file path preview fields are enabled.
        if cls._canBeRelative:
            cls.connectToDialogControls(parentLayout)

        if cls._haveRelativePathFields:
            # Only enable fields when make relative is checked.
            makePathRelative = cmds.checkBox(cls.kMakePathRelativeCheckBox, query=True, value=True)
            cls.onMakePathRelativeChanged(makePathRelative)

    @classmethod
    def uiCommit(cls, parentLayout, relativeToWhat):
        """
        Helper method to commit the UI layout for the file relative actions.

        Input parentLayout arg is expected to the a scroll layout into which controls
        can be added.

        Input relativeToWhat tells what the file is relative to. See the class docs.
        """
        cmds.setParent(parentLayout)

        # Get the current checkbox state and save to optionVar.
        relative = cmds.checkBox(cls.kMakePathRelativeCheckBox, query=True, value=True)
        cmds.optionVar(iv=('mayaUsd_MakePathRelativeTo' + relativeToWhat, relative))

    @classmethod
    def connectToDialogControls(cls, parentLayout):
        """
        Find some of the conrols (such as file name edit field) in the dialog
        (under the input parent layout) and connect to them for change notifs.
        Used so we can update the file path preview fields.
        """

        # Get the Qt pointer for the input parent string (layout from mel).
        ptr = omui.MQtUtil.findLayout(parentLayout)
        if ptr is not None:
            # Find the top-level window from that parent layout.
            maya_widget = wrapInstance(int(ptr), QWidget)
            pp = maya_widget.parent()
            while pp:
                if pp.inherits('QFileDialog'):
                    cls._fileDialog = pp
                pp = pp.parent()
                if pp:
                    maya_window = pp

        if maya_window:
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
        cmds.textFieldGrp(cls.kResolvedPathTextField, edit=True, enable=enableFields, text='')
        if enableFields:
            cls.updateFilePathPreviewFields(checked)

    @classmethod
    def onfileNameEditFieldChanged(cls, text):
        """Callback from Qt textChanged event from fileNameEditField."""
        cls.updateFilePathPreviewFields()

    @classmethod
    def onLookinTextChanged(cls, text):
        """Callback from Qt currentTextChanged from LookIn combobox."""
        cls.updateFilePathPreviewFields()

    @classmethod
    def updateFilePathPreviewFields(cls, makePathRelative=None):
        if not cls._haveRelativePathFields:
            return
        if makePathRelative == None:
            makePathRelative = cmds.checkBox(cls.kMakePathRelativeCheckBox, query=True, value=True)

        if cls._canBeRelative and makePathRelative:
            # If the accept button is disabled it means there is no valid input in the file
            # name edit field.
            selFiles = cls._fileDialog.selectedFiles() if cls._fileDialog and cls._acceptButton and cls._acceptButton.isEnabled() else None
            selectedFile = selFiles[0] if selFiles else ''

            # Make sure the file path has a valid USD extension. This is NOT done by the fileDialog so
            # the user is free to enter any extension they want. The layer editor code will then verify
            # (and fix if needed) the file path before saving. We do the same here for preview.
            unresolvedPath = mayaUsdLib.Util.ensureUSDFileExtension(selectedFile) if selectedFile else ''
            relativePath = ''
            if unresolvedPath and cls._parentLayerPath:
                relativePath = mayaUsdLib.Util.getPathRelativeToDirectory(unresolvedPath, cls._parentLayerPath)
            elif unresolvedPath:
                relativePath = mayaUsdLib.Util.getPathRelativeToMayaSceneFile(unresolvedPath)
            cmds.textFieldGrp(cls.kUnresolvedPathTextField, edit=True, text=relativePath)
            cmds.textFieldGrp(cls.kResolvedPathTextField, edit=True, text=unresolvedPath)

    @classmethod
    def selectionChanged(cls, parentLayout, selection):
        """Callback from fileDialog selectionChanged."""
        cmds.setParent(parentLayout)
        cls.updateFilePathPreviewFields()

    @classmethod
    def fileTypeChanged(cls, parentLayout, newType):
        """Callback from fileDialog command fileTypeChanged."""
        cmds.setParent(parentLayout)
        cls.updateFilePathPreviewFields()

class usdRootFileRelative(usdFileRelative):
    '''
    Helper class to create the UI for load/save dialog boxes that need to make the
    selected file name optionally relative to the Maya scene file.
    '''

    kRelativeToWhat = 'SceneFile'

    @classmethod
    def uiCreate(cls, parentLayout):
        super(usdRootFileRelative, cls).uiCreate(parentLayout, cls.kRelativeToWhat)

    @classmethod
    def uiInit(cls, parentLayout, filterType):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
        # If there is no Maya scene file saved, then the checkbox and label should be disabled.
        haveSceneFile = cmds.file(q=True, exists=True)
        cls.setRelativeFilePathRoot(cmds.file(query=True, sceneName=True))
        super(usdRootFileRelative, cls).uiInit(parentLayout, haveSceneFile, cls.kRelativeToWhat)

    @classmethod
    def uiCommit(cls, parentLayout, selectedFile=None):
        '''
        Note: the function takes an unused selectedFile argument to be compatible
              with the dialog2 command API.
        '''
        super(usdRootFileRelative, cls).uiCommit(parentLayout, cls.kRelativeToWhat)


class usdSubLayerFileRelative(usdFileRelative):
    '''
    Helper class to create the UI for load/save dialog boxes that need to make the
    selected file name optionally relative to a parent layer.
    '''

    kRelativeToWhat = 'ParentLayer'

    @classmethod
    def uiCreate(cls, parentLayout):
        super(usdSubLayerFileRelative, cls).uiCreate(parentLayout, cls.kRelativeToWhat)

    @classmethod
    def uiInit(cls, parentLayout, filterType, parentLayerPath = ""):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''

        cls._parentLayerPath = parentLayerPath
        canBeRelative = bool(usdFileRelative.getRelativeFilePathRoot())
        super(usdSubLayerFileRelative, cls).uiInit(parentLayout, canBeRelative, cls.kRelativeToWhat)

    @classmethod
    def uiCommit(cls, parentLayout, selectedFile=None):
        '''
        Note: the function takes an unused selectedFile argument to be compatible
              with the dialog2 command API.
        '''
        super(usdSubLayerFileRelative, cls).uiCommit(parentLayout, cls.kRelativeToWhat)


class usdFileRelativeToEditTargetLayer(usdFileRelative):
    '''
    Helper class to create the UI for load/save dialog boxes that need to make the
    selected file name optionally relative to a layer file.
    '''

    kRelativeToWhat = 'EditTargetLayer'

    @classmethod
    def uiCreate(cls, parentLayout):
        super(usdFileRelativeToEditTargetLayer, cls).uiCreate(parentLayout, cls.kRelativeToWhat)

    @classmethod
    def uiInit(cls, parentLayout, filterType):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
        # If there is no target layer saved, then the checkbox and label should be disabled.
        canBeRelative = bool(usdFileRelative.getRelativeFilePathRoot())
        super(usdFileRelativeToEditTargetLayer, cls).uiInit(parentLayout, canBeRelative, cls.kRelativeToWhat)

    @classmethod
    def uiCommit(cls, parentLayout, selectedFile=None):
        '''
        Note: the function takes an unused selectedFile argument to be compatible
              with the dialog2 command API.
        '''
        super(usdFileRelativeToEditTargetLayer, cls).uiCommit(parentLayout, cls.kRelativeToWhat)
