import pxr.UsdQtEditors.opinionEditor as oe
from pxr.UsdQtEditors.opinionEditor import OpinionDialog

import ufe
usdRtid = ufe.RunTimeMgr.instance().getId('USD')


def getPrimsFromUfe():
    globalSelection = ufe.GlobalSelection.get()
    prims = []
    for ufeItem in globalSelection:
        if ufeItem.runTimeId() == usdRtid:
            prims.append(ufeItem.prim())
    return prims


def getMayaWindow():
    import maya.OpenMayaUI as omUI
    import shiboken2
    import PySide2.QtWidgets

    ptr = omUI.MQtUtil.mainWindow()
    return shiboken2.wrapInstance(long(ptr), PySide2.QtWidgets.QMainWindow)


class UfeOpinionDialog(OpinionDialog):
    class SelectionObserver(ufe.Observer):
        # for some reason, adding an __init__ makes addObserver error with:
        # RuntimeError: Unable to cast from non-held to held instance (T& to Holder<T>) (compile in debug mode for type information) #
        # ...so we have to set .opinionDialog "manually", after creation...

        def __call__(self, notification):
            opinionDialog = getattr(self, 'opinionDialog', None)
            if opinionDialog:
                opinionDialog.controller.ResetPrims(getPrimsFromUfe())

    def __init__(self, parent=None):
        self.selObserver = None
        self.globalSelection = None
        super(UfeOpinionDialog, self).__init__(prims=getPrimsFromUfe(), parent=parent)
        self.addSelObserver()

    def addSelObserver(self):
        self.globalSelection = ufe.GlobalSelection.get()
        if self.globalSelection:
            self.selObserver = self.SelectionObserver()
            # if you add a __init__ or __new__, the selObserver doesn't work!
            # ...so manually adding an attr outside of __init__
            self.selObserver.opinionDialog = self
            self.globalSelection.addObserver(self.selObserver)

    def removeSelObserver(self):
        if self.globalSelection and self.selObserver:
            self.globalSelection.removeObserver(self.selObserver)
            self.selObserver = None
            self.globalSelection = None

    def reject(self, *args, **kwargs):
        self.removeSelObserver()
        super(UfeOpinionDialog, self).reject(*args, **kwargs)

    @classmethod
    def launch(cls):
        dialog = cls(parent=getMayaWindow())
        dialog.setWindowTitle("USD Inspector")
        dialog.show()
        dialog.raise_()
        dialog.activateWindow()
        return dialog
