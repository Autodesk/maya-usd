import maya.cmds as cmds
import maya.mel as mel
from mayaUSDRegisterStrings import getMayaUsdString
from mayaUsdMayaReferenceUtils import pushOptionsUITemplate

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

    kMakePathRelativeCheckBox = 'MakePathRelativeTo'

    _relativeToFilePath = None

    @classmethod
    def setRelativeFilePathRoot(cls, filePath):
        '''
        Sets to which file the relative file will be anchored.
        Set to empty or None to have the file be absolute, not relative.
        '''
        cls._relativeToFilePath = filePath

    @classmethod
    def getRelativeFilePathRoot(cls):
        '''
        Gets to which file the relative file will be anchored.
        Empty or None to have the file be absolute, not relative.
        '''
        return cls._relativeToFilePath

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
 
        optBoxMarginWidth = mel.eval('global int $gOptionBoxTemplateDescriptionMarginWidth; $gOptionBoxTemplateDescriptionMarginWidth += 0')
        cmds.setParent(topForm)
        cmds.frameLayout(label=kFileOptionsStr, collapsable=False)
        widgetColumn = cmds.columnLayout()
        cmds.checkBox(cls.kMakePathRelativeCheckBox + relativeToWhat, label=kMakePathRelativeStr, ann=kMakePathRelativeAnnStr)

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

        # Get the current checkbox value from optionVar (if any) and update checkbox.
        if cmds.optionVar(exists='mayaUsd_MakePathRelativeTo' + relativeToWhat):
            relative = cmds.optionVar(query='mayaUsd_MakePathRelativeTo' + relativeToWhat)
            cmds.checkBox(cls.kMakePathRelativeCheckBox + relativeToWhat, edit=True, value=relative)

        # If if cannot be relative, then the checkbox and label should be disabled.
        cmds.checkBox(cls.kMakePathRelativeCheckBox + relativeToWhat, edit=True, enable=canBeRelative)

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
        relative = cmds.checkBox(cls.kMakePathRelativeCheckBox + relativeToWhat, query=True, value=True)
        cmds.optionVar(iv=('mayaUsd_MakePathRelativeTo' + relativeToWhat, relative))


class usdRootFileRelative(usdFileRelative):
    '''
    Helper class to create the UI for load/save dialog boxes that need to make the
    selected file name optionally relative to the Maya scene file.
    '''

    kRelativeToWhat = 'SceneFile'

    @classmethod
    def uiCreate(cls, parentLayout):
        usdFileRelative.uiCreate(parentLayout, cls.kRelativeToWhat)

    @classmethod
    def uiInit(cls, parentLayout, filterType):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
        # If there is no Maya scene file saved, then the checkbox and label should be disabled.
        haveSceneFile = cmds.file(q=True, exists=True)
        usdFileRelative.setRelativeFilePathRoot(cmds.file(query=True, sceneName=True))
        usdFileRelative.uiInit(parentLayout, haveSceneFile, cls.kRelativeToWhat)

    @classmethod
    def uiCommit(cls, parentLayout, selectedFile=None):
        '''
        Note: the function takes an unused selectedFile argument to be compatible
              with the dialog2 command API.
        '''
        usdFileRelative.uiCommit(parentLayout, cls.kRelativeToWhat)


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
    def uiInit(cls, parentLayout, filterType):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
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
        usdFileRelative.uiCreate(parentLayout, cls.kRelativeToWhat)

    @classmethod
    def uiInit(cls, parentLayout, filterType):
        '''
        Note: the function takes an unused filterType argument to be compatible
              with the dialog2 command API.
        '''
        # If there is no target layer saved, then the checkbox and label should be disabled.
        canBeRelative = bool(usdFileRelative.getRelativeFilePathRoot())
        usdFileRelative.uiInit(parentLayout, canBeRelative, cls.kRelativeToWhat)

    @classmethod
    def uiCommit(cls, parentLayout, selectedFile=None):
        '''
        Note: the function takes an unused selectedFile argument to be compatible
              with the dialog2 command API.
        '''
        usdFileRelative.uiCommit(parentLayout, cls.kRelativeToWhat)
