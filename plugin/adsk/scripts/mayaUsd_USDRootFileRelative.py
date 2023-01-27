import maya.cmds as cmds
import maya.mel as mel
from mayaUSDRegisterStrings import getMayaUsdString
from mayaUsdMayaReferenceUtils import pushOptionsUITemplate

class usdRootFileRelative(object):
    fileNameEditField = None

    kMakePathRelativeCheckBox = 'MakePathRelative'

    @classmethod
    def uiCreate(cls, parentLayout):
        """
        Helper method to create the UI layout for the USD root file relative actions.

        Input parentLayout arg is expected to the a scroll layout into which controls
        can be added.
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
        kMakePathRelativeStr = getMayaUsdString("kMakePathRelativeToSceneFile")
 
        optBoxMarginWidth = mel.eval('global int $gOptionBoxTemplateDescriptionMarginWidth; $gOptionBoxTemplateDescriptionMarginWidth += 0')
        cmds.setParent(topForm)
        cmds.frameLayout(label=kFileOptionsStr, collapsable=False)
        widgetColumn = cmds.columnLayout()
        cmds.checkBox(cls.kMakePathRelativeCheckBox, label=kMakePathRelativeStr)

    @classmethod
    def uiInit(cls, parentLayout, filterType):
        cmds.setParent(parentLayout)

        # Get the current checkbox value from optionVar (if any) and update checkbox.
        if cmds.optionVar(exists='mayaUsd_MakePathRelativeToSceneFile'):
            relative = cmds.optionVar(query='mayaUsd_MakePathRelativeToSceneFile')
            cmds.checkBox(cls.kMakePathRelativeCheckBox, edit=True, value=relative)

        # If there is no Maya scene file saved, then the checkbox and label should be disabled.
        haveSceneFile = cmds.file(q=True, exists=True)
        cmds.checkBox(cls.kMakePathRelativeCheckBox, edit=True, enable=haveSceneFile)

    @classmethod
    def uiCommit(cls, parentLayout, selectedFile=None):
        cmds.setParent(parentLayout)

        # Get the current checkbox state and save to optionVar.
        relative = cmds.checkBox(cls.kMakePathRelativeCheckBox, query=True, value=True)
        cmds.optionVar(iv=('mayaUsd_MakePathRelativeToSceneFile', relative))
