import maya.cmds as cmds
from pxr import Usd

class LightLinkingCustomControl(object):
    '''Custom control for the light linking data we want to display.'''
    def __init__(self, item, prim, useNiceName):
        # In Maya 2022.1 we need to hold onto the Ufe SceneItem to make
        # sure it doesn't go stale. This is not needed in latest Maya.
        super(LightLinkingCustomControl, self).__init__()
        mayaVer = '%s.%s' % (cmds.about(majorVersion=True), cmds.about(minorVersion=True))
        self.item = item if mayaVer == '2022.1' else None
        self.prim = prim
        self.useNiceName = useNiceName

    def onCreate(self, *args):
        if self.prim.IsValid() == True and self.prim.HasAPI('CollectionAPI', 'lightLink'):
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

                from usd_shared_components.collection.widget import CollectionWidget # type: ignore

                self.widget = CollectionWidget(Usd.CollectionAPI.Get(self.prim, 'lightLink'))
                parentWidget.layout().addWidget(self.widget)
                
            except Exception as ex:
                print(ex)

            self.refresh()

    def onReplace(self, *args):
        # Nothing needed here since USD data is not time varying. Normally this template
        # is force rebuilt all the time, except in response to time change from Maya. In
        # that case we don't need to update our controls since none will change.
        pass

    def refresh(self):
        pass