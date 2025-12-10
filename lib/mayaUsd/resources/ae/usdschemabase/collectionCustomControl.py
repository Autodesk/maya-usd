import maya.cmds as cmds
from pxr import Usd

from usd_shared_components.collection.widget import CollectionWidget # type: ignore
from usd_shared_components.common.host import Host # type: ignore
from usd_shared_components.common.theme import Theme # type: ignore
from usd_shared_components.common.persistentStorage import PersistentStorage # type: ignore

from .collectionMayaHost import MayaHost, MayaTheme, MayaPersistentStorage

class CollectionCustomControl(object):
    '''Custom control for the light linking data we want to display.'''

    @staticmethod
    def isCollectionAttribute(aeTemplate, attrName):
        '''
        Verify if the given attribute name is a collection attribute.
        '''
        attrNameParts = attrName.split(':')
        if len(attrNameParts) <= 1:
            return False
        
        instanceName = attrNameParts[1]
        if not aeTemplate.prim.HasAPI(Usd.CollectionAPI, instanceName):
            return False
        
        if len(attrNameParts) > 2:
            aeTemplate.suppress(attrName)
            return False

        return True

    @staticmethod
    def creator(aeTemplate, attrName, label=None):
        '''
        If the attribute is a collection attribute then create a section to edit it.
        '''
        if CollectionCustomControl.isCollectionAttribute(aeTemplate, attrName):
            attrName, instanceName = attrName.split(':')
            return CollectionCustomControl(aeTemplate.item, aeTemplate.prim, attrName, instanceName, aeTemplate.useNiceName, label=label)
        else:
            return None

    def __init__(self, item, prim, attrName, instanceName, useNiceName, label=None):
        # In Maya 2022.1 we need to hold onto the Ufe SceneItem to make
        # sure it doesn't go stale. This is not needed in latest Maya.
        super(CollectionCustomControl, self).__init__()
        mayaVer = '%s.%s' % (cmds.about(majorVersion=True), cmds.about(minorVersion=True))
        self.item = item if mayaVer == '2022.1' else None
        self.attrName = attrName
        self.instanceName = instanceName
        self.prim = prim
        self.useNiceName = useNiceName
        self.label = label

    def onCreate(self, *args):
        try:
            try:
                from shiboken6 import wrapInstance
                from PySide6.QtWidgets import QWidget
            except:
                from shiboken2 import wrapInstance # type: ignore
                from PySide2.QtWidgets import QWidget # type: ignore
                
            from maya.OpenMayaUI import MQtUtil

            self.parent = cmds.setParent(q=True)
            ptr = MQtUtil.findControl(self.parent)
            parentWidget = wrapInstance(int(ptr), QWidget)

            if Host._instance is None:
                Host.injectInstance(MayaHost())
                Theme.injectInstance(MayaTheme())
                PersistentStorage.injectInstance(MayaPersistentStorage())
            self.widget = CollectionWidget(self.prim, Usd.CollectionAPI.Get(self.prim, self.instanceName))
            parentWidget.layout().addWidget(self.widget)

        except Exception as ex:
            print('Failed to create collection custom control: %s' % (ex))

        self.refresh()

    def onReplace(self, *args):
        # Nothing needed here since USD data is not time varying. Normally this template
        # is force rebuilt all the time, except in response to time change from Maya. In
        # that case we don't need to update our controls since none will change.
        pass

    def refresh(self):
        pass