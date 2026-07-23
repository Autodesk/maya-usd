


from .attributeCustomControl import AttributeCustomControl
from .attributeCustomControl import cleanAndFormatTooltip

import ufe
import mayaUsd.ufe as mayaUsdUfe
import mayaUsd.lib as mayaUsdLib
import maya.cmds as cmds
import maya.internal.ufeSupport.attributes as attributes

from maya.common.ui import LayoutManager, ParentManager

from pxr import Sdf

class RelationshipCustomControl(AttributeCustomControl):

    relationshipField = "UIRelationshipField"
    relationshipFieldLabel = "UIRelationshipFieldLabel"

    @staticmethod
    def creator(aeTemplate, attrName, label=None):
        # is th input attribute a relationship?
        ufeAttr = aeTemplate.attrs.attribute(attrName)
        # get the usd property

        usdPrim = mayaUsdUfe.getPrimFromRawItem(aeTemplate.item.getRawAddress())
        usdRel = usdPrim.GetRelationship(attrName)
        if usdRel.IsValid():
            return RelationshipCustomControl(aeTemplate.item, ufeAttr, aeTemplate.prim, attrName, aeTemplate.useNiceName, label=label)
        else:
            return None

    def __init__(self, ufeItem, ufeAttr, prim, attrName, useNiceName, label=None):
        super(RelationshipCustomControl, self).__init__(ufeAttr, attrName, useNiceName, label=label)
        self.path = ufeItem.path()
        self.prim = prim
        self.usdPrim = mayaUsdUfe.getPrimFromRawItem(ufeItem.getRawAddress())
        self.usdRel = self.usdPrim.GetRelationship(attrName)

    def onCreate(self, *args):
        cmds.setUITemplate("attributeEditorTemplate", pst=True)
        # Create the control.
        attrLabel = self.getUILabel()
        # for now just a simple textfield
        createdControl = cmds.rowLayout(nc=5, adj=3)
        self.controlName = createdControl
        with LayoutManager(createdControl):
            cmds.text(RelationshipCustomControl.relationshipFieldLabel, al='right', label=attrLabel)
            self.uiControl = cmds.textField(RelationshipCustomControl.relationshipField)
        
        self.onReplace()
        return createdControl
    
    def getUFEFullPath(self, ufeAttr):
        return '%s.%s' % (ufe.PathString.string(ufeAttr.sceneItem().path()), ufeAttr.name)
    
    def onReplace(self, *args):
        ufeAttr = self.ufeAttr
        controlName = self.controlName
        with ParentManager(controlName):
            self.updateUi(ufeAttr, controlName)

    def updateUi(self, attr, uiControlName):
        '''Callback function to update the UI when the data model changes.'''
        isLocked = attributes.isAttributeLocked(attr)
        bgClr = attributes.getAttributeColorRGB(attr)
        value = self.getValue(attr)
        # We might get called from the text field instead of the row layout:
        fieldPos = uiControlName.find(RelationshipCustomControl.relationshipField)
        if fieldPos > 0:
            uiControlName = uiControlName[:fieldPos - 1]
        with ParentManager(uiControlName):
            args = dict(
                text=value,
                enable=not isLocked,
                changeCommand=lambda value : self.setValue(attr, value)
            )
            cmds.textField(self.uiControl, e=True, **args)
            if bgClr:
                cmds.textField(self.uiControl, e=True, backgroundColor=bgClr)
            
    def setValue(self, ufeAttr, value):
        targets = [Sdf.Path(target) for target in value.split(" ")]
        if mayaUsdUfe.isRelationshipEditAllowed(self.usdRel, targets, []):
            self.usdRel.SetTargets(targets)
            return True 
        else:
            cmds.error("Edits not allowed, attribute is set in a stronger layer")
        return False
    
    def getValue(self, ufeAttr):
        targets = self.usdRel.GetTargets()
        result = " ".join([target.pathString for target in targets])
        return result