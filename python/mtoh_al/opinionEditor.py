#
# Copyright 2019 Luma Pictures
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

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
    import PySide2.QtWidgets as QtWidgets

    ptr = omUI.MQtUtil.mainWindow()
    return shiboken2.wrapInstance(long(ptr), QtWidgets.QMainWindow)


class UfeOpinionDialog(OpinionDialog):
    class SelectionObserver(ufe.Observer):
        # for some reason, adding an __init__ makes addObserver error with:
        # RuntimeError: Unable to cast from non-held to held instance (T& to Holder<T>) (compile in debug mode for type information) #
        # ...so we have to set .opinionDialog "manually", after creation...

        def __call__(self, notification):
            #print "SelectionObserver()"
            if not isinstance(notification, ufe.SelectionChanged):
                return
            opinionDialog = getattr(self, 'opinionDialog', None)
            if opinionDialog:
                #print "...resetting prims!"
                opinionDialog.controller.ResetPrims(getPrimsFromUfe())

    def __init__(self, parent=None):
        self.selObserver = None
        self.globalSelection = None
        super(UfeOpinionDialog, self).__init__(prims=getPrimsFromUfe(), parent=parent)
        self.addSelObserver()
        self.destroyed.connect(self.removeSelObserver)
        self.finished.connect(self.removeSelObserver)

    def addSelObserver(self):
        #print "addSelObserver()"
        self.globalSelection = ufe.GlobalSelection.get()
        if self.globalSelection is not None:
            self.selObserver = self.SelectionObserver()
            #print "set self.selObserver"
            # if you add a __init__ or __new__, the selObserver doesn't work!
            # ...so manually adding an attr outside of __init__
            self.selObserver.opinionDialog = self
            self.globalSelection.addObserver(self.selObserver)

    def removeSelObserver(self, *args, **kwargs):
        #print "removeSelObserver()"
        if self.globalSelection is not None or self.selObserver is not None:
            if self.globalSelection is not None and self.selObserver is not None:
                self.globalSelection.removeObserver(self.selObserver)
            self.selObserver = None
            self.globalSelection = None
            #print "unset self.selObserver"

    @classmethod
    def launch(cls):
        import PySide2.QtCore as QtCore

        dialog = cls(parent=getMayaWindow())
        dialog.setWindowTitle("USD Inspector")
        dialog.setAttribute(QtCore.Qt.WA_DeleteOnClose, True)
        dialog.show()
        dialog.raise_()
        dialog.activateWindow()
        return dialog
