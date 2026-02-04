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

import mayaUsd.lib as mayaUsdLib
import mayaUsd.ufe as mayaUsdUfe
from mayaUsdLibRegisterStrings import getMayaUsdLibString

import usdUfe

from pxr import Usd, Kind

class MetadataCustomControl(object):
    '''Custom control for all prim metadata we want to display.'''
    
    def __init__(self, item, prim, useNiceName):
        # In Maya 2022.1 we need to hold onto the Ufe SceneItem to make
        # sure it doesn't go stale. This is not needed in latest Maya.
        super(MetadataCustomControl, self).__init__()
        mayaVer = '%s.%s' % (cmds.about(majorVersion=True), cmds.about(minorVersion=True))
        self.item = item if mayaVer == '2022.1' else None
        self.prim = prim
        self.useNiceName = useNiceName

        # There are four metadata that we always show: primPath, kind, active, instanceable
        # We use a dictionary to store the various other metadata that this prim contains.
        self.extraMetadata = dict()

    def onCreate(self, *args):
        # Metadata: PrimPath
        # The prim path is for display purposes only - it is not editable, but we
        # allow keyboard focus so you copy the value.
        self.primPath = cmds.textFieldGrp(label='Prim Path', editable=False, enableKeyboardFocus=True)

        # Metadata: Kind
        # We add the known Kind types, in a certain order ("model hierarchy") and then any
        # extra ones that were added by extending the kind registry.
        # Note: we remove the "model" kind because in the USD docs it states, 
        #       "No prim should have the exact kind "model".
        allKinds = Kind.Registry.GetAllKinds()
        allKinds.remove(Kind.Tokens.model)
        knownKinds = [Kind.Tokens.group, Kind.Tokens.assembly, Kind.Tokens.component, Kind.Tokens.subcomponent]
        temp1 = [ele for ele in allKinds if ele not in knownKinds]
        knownKinds.extend(temp1)

        # If this prim's kind is not registered, we need to manually
        # add it to the list.
        model = Usd.ModelAPI(self.prim)
        primKind = model.GetKind()
        if primKind not in knownKinds:
            knownKinds.insert(0, primKind)
        if '' not in knownKinds:
            knownKinds.insert(0, '')    # Set metadata value to "" (or empty).

        self.kind = cmds.optionMenuGrp(label='Kind',
                                       cc=self._onKindChanged,
                                       ann=getMayaUsdLibString('kKindMetadataAnn'))

        for ele in knownKinds:
            cmds.menuItem(label=ele)

        # Metadata: Active
        self.active = cmds.checkBoxGrp(label='Active',
                                       ncb=1,
                                       cc1=self._onActiveChanged,
                                       ann=getMayaUsdLibString('kActiveMetadataAnn'))

        # Metadata: Instanceable
        self.instan = cmds.checkBoxGrp(label='Instanceable',
                                       ncb=1,
                                       cc1=self._onInstanceableChanged,
                                       ann=getMayaUsdLibString('kInstanceableMetadataAnn'))

        # Get all the other Metadata and remove the ones above, as well as a few
        # we don't ever want to show.
        allMetadata = self.prim.GetAllMetadata()
        keysToDelete = ['kind', 'active', 'instanceable', 'typeName', 'documentation', 'assetInfo']
        for key in keysToDelete:
            allMetadata.pop(key, None)
        if allMetadata:
            cmds.separator(h=10, style='single', hr=True)

            for k in allMetadata:
                # All extra metadata is for display purposes only - it is not editable, but we
                # allow keyboard focus so you copy the value.
                mdLabel = mayaUsdUfe.prettifyName(k) if self.useNiceName else k
                self.extraMetadata[k] = cmds.textFieldGrp(label=mdLabel, editable=False, enableKeyboardFocus=True)

        # Update all metadata values.
        self.refresh()

    def onReplace(self, *args):
        # Nothing needed here since USD data is not time varying. Normally this template
        # is force rebuilt all the time, except in response to time change from Maya. In
        # that case we don't need to update our controls since none will change.
        pass

    def _refreshKind(self):
        model = Usd.ModelAPI(self.prim)
        primKind = model.GetKind()
        if not primKind:
            # Special case to handle the empty string (for meta data value empty).
            cmds.optionMenuGrp(self.kind, edit=True, select=1)
        else:
            cmds.optionMenuGrp(self.kind, edit=True, value=primKind)

    def refresh(self):
        # PrimPath
        cmds.textFieldGrp(self.primPath, edit=True, text=str(self.prim.GetPath()))

        # Kind
        self._refreshKind()

        # Active
        cmds.checkBoxGrp(self.active, edit=True, value1=self.prim.IsActive())

        # Instanceable
        cmds.checkBoxGrp(self.instan, edit=True, value1=self.prim.IsInstanceable())

        # All other metadata types
        for k in self.extraMetadata:
            v = self.prim.GetMetadata(k) if k != 'customData' else self.prim.GetCustomData()
            cmds.textFieldGrp(self.extraMetadata[k], edit=True, text=str(v))

    def _onKindChanged(self, value):
        with mayaUsdLib.UsdUndoBlock():
            try:
                usdUfe.SetKindCommand(self.prim, value).execute()
            except Exception as ex:
                # Note: the command might not work because there is a stronger
                #       opinion or an editRouting prevention so update the option menu
                self._refreshKind()
                cmds.error(str(ex))


    def _onActiveChanged(self, value):
        with mayaUsdLib.UsdUndoBlock():
            try:
                usdUfe.ToggleActiveCommand(self.prim).execute()
            except Exception as ex:
                # Note: the command might not work because there is a stronger
                #       opinion, so update the checkbox.
                cmds.checkBoxGrp(self.active, edit=True, value1=self.prim.IsActive())
                cmds.error(str(ex))

    def _onInstanceableChanged(self, value):
        with mayaUsdLib.UsdUndoBlock():
            try:
                usdUfe.ToggleInstanceableCommand(self.prim).execute()
            except Exception as ex:
                # Note: the command might not work because there is a stronger
                #       opinion, so update the checkbox.
                cmds.checkBoxGrp(self.instan, edit=True, value1=self.prim.IsInstanceable())
                cmds.error(str(ex))
