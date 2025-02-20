# Copyright 2024 Autodesk
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

import maya.cmds as cmds

import ufe

from pxr import Usd, UsdGeom, UsdShade, Tf


# Manage the refresh of teh attribute editor so that we only queue one refresh
# at a time
_editorRefreshQueued = False

def _refreshEditor():
    '''Reset the queued refresh flag and refresh the AE.'''
    global _editorRefreshQueued
    _editorRefreshQueued = False
    cmds.refreshEditorTemplates()
    
def _queueEditorRefresh():
    '''If there is not already a AE refresh queued, queue a refresh.'''
    global _editorRefreshQueued
    if _editorRefreshQueued:
        return
    cmds.evalDeferred(_refreshEditor, low=True)
    _editorRefreshQueued = True


# These Attribute Editor custom controls have no UI.
# They observe UFE or listen to USD to refresh the
# AE (attribute editor) when needed.


class UfeAttributesObserver(ufe.Observer):
    '''
    Custom control, but does not have any UI. Instead we use
    this control to be notified from UFE when any attribute has changed
    so we can update the AE.
    '''

    _watchedAttrs = {
        # This is to fix refresh issue when transform is added to a prim.
        UsdGeom.Tokens.xformOpOrder,
        # This is to fix refresh issue when an existing material is assigned
        # to the prim when it already had a another material.
        UsdShade.Tokens.materialBinding,
    }

    def __init__(self, item):
        super(UfeAttributesObserver, self).__init__()
        self._item = item

    def __del__(self):
        ufe.Attributes.removeObserver(self)

    def __call__(self, notification):
        refreshEditor = False
        if isinstance(notification, ufe.AttributeValueChanged):
            if notification.name() in UfeAttributesObserver._watchedAttrs:
                refreshEditor = True
        if hasattr(ufe, "AttributeAdded") and isinstance(notification, ufe.AttributeAdded):
            refreshEditor = True
        if hasattr(ufe, "AttributeRemoved") and isinstance(notification, ufe.AttributeRemoved):
            refreshEditor = True
        if refreshEditor:
            _queueEditorRefresh()


    def onCreate(self, *args):
        ufe.Attributes.addObserver(self._item, self)

    def onReplace(self, *args):
        # Nothing needed here since we don't create any UI.
        pass

class UfeConnectionChangedObserver(ufe.Observer):
    '''
    Custom control, but does not have any UI. Instead we use
    this control to be notified from UFE when any attribute has changed
    so we can update the AE.
    '''
    
    def __init__(self, item):
        super(UfeConnectionChangedObserver, self).__init__()
        self._item = item

    def __del__(self):
        ufe.Attributes.removeObserver(self)

    def __call__(self, notification):
        if hasattr(ufe, "AttributeConnectionChanged") and isinstance(notification, ufe.AttributeConnectionChanged):
            _queueEditorRefresh()

    def onCreate(self, *args):
        ufe.Attributes.addObserver(self._item, self)

    def onReplace(self, *args):
        # Nothing needed here since we don't create any UI.
        pass


class UsdNoticeListener(object):
    '''
    Inserted as a custom control, but does not have any UI. Instead we use
    this control to be notified from USD when the prim has changed
    so we can update the AE fields.
    '''
    
    def __init__(self, prim, aeControls):
        self.prim = prim
        self.aeControls = aeControls

    def onCreate(self, *args):
        self.listener = Tf.Notice.Register(Usd.Notice.ObjectsChanged,
                                           self.__OnPrimsChanged, self.prim.GetStage())
        pname = cmds.setParent(q=True)
        cmds.scriptJob(uiDeleted=[pname, self.onClose], runOnce=True)

    def onReplace(self, *args):
        # Nothing needed here since we don't create any UI.
        pass

    def onClose(self):
        if self.listener:
            self.listener.Revoke()
            self.listener = None

    def __OnPrimsChanged(self, notice, sender):
        if notice.HasChangedFields(self.prim):
            # Iterate thru all the AE controls (we were given when created) and
            # call the refresh method (if it exists).
            for ctrl in self.aeControls:
                if hasattr(ctrl, 'refresh'):
                    ctrl.refresh()
