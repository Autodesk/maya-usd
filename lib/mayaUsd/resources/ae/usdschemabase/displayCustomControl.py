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

from mayaUsdLibRegisterStrings import getMayaUsdLibString
import mayaUsd.ufe as mayaUsdUfe
import mayaUsd.lib as mayaUsdLib

import maya.cmds as cmds
import maya.mel as mel
from maya.internal.ufeSupport import ufeCmdWrapper as ufeCmd

import ufe

from pxr import Gf, Sdf

import contextlib


@contextlib.contextmanager
def PrimCustomDataEditRouting(prim, *args):
    '''
    A context manager that activates prim customData editRouting.
    '''
    # Note: the edit router context must be kept alive in a variable.
    ctx = mayaUsdUfe.PrimMetadataEditRouterContext(prim, Sdf.PrimSpec.CustomDataKey, *args)
    yield


class DisplayCustomControl(object):
    '''Custom control for adding in extra display custom data.'''
    def __init__(self, item, prim):
        super(DisplayCustomControl, self).__init__()
        self.item = item
        self.prim = prim

        # Check whether Ufe has the scene item meta data support.
        self.useMetadata = hasattr(ufe.SceneItem, "getGroupMetadata") and hasattr(ufe.Value, "typeName")

    @property
    def GROUP(self):
        # When using meta data we use a special group name which will automatically
        # write to the session layer.
        return 'SessionLayer-Autodesk' if self.useMetadata else 'Autodesk'

    @property
    def USE_OUTLINER_COLOR(self):
        # To be more generic and not Maya specific we write out the custom data
        # as "Text Color" instead of "Outliner Color".
        key = 'Use Text Color'
        return key if self.useMetadata else (self.GROUP + ':' + key)

    @property
    def OUTLINER_COLOR(self):
        key = 'Text Color'
        return key if self.useMetadata else (self.GROUP + ':' + key)

    def onCreate(self, *args):
        '''Add two fields for outliner color to match Maya objects.'''
        l1 = mel.eval('uiRes(\"m_AEdagNodeCommon.kUseOutlinerColor\");')
        l2 = mel.eval('uiRes(\"m_AEdagNodeCommon.kOutlinerColor\");')
        self.useOutlinerColor = cmds.checkBoxGrp(label1=l1, ncb=1,
                                                 ann=getMayaUsdLibString('kUseOutlinerColorAnn'),
                                                 cc1=self._onUseOutlinerColorChanged)
        self.outlinerColor = cmds.colorSliderGrp(label=l2,
                                                 ann=getMayaUsdLibString('kOutlinerColorAnn'),
                                                 cc=self._onOutlinerColorChanged)

        # Update all the custom data controls.
        self.refresh()

    def onReplace(self, *args):
        pass

    def _clearUseOutlinerColor(self):
        cmds.checkBoxGrp(self.useOutlinerColor, edit=True, v1=False)

    def _clearOutlinerColor(self):
        cmds.colorSliderGrp(self.outlinerColor, edit=True, rgb=(0,0,0))
        
    def clear(self):
        self._clearUseOutlinerColor()
        self._clearOutlinerColor()

    def refresh(self):
        try:
            if self.useMetadata:
                # Use Ufe SceneItem metadata to get values.
                useOutlinerColor = self.item.getGroupMetadata(self.GROUP, self.USE_OUTLINER_COLOR)
                if not useOutlinerColor.empty() and (useOutlinerColor.typeName() == 'bool'):
                    cmds.checkBoxGrp(self.useOutlinerColor, edit=True, v1=bool(useOutlinerColor))
                else:
                    self._clearUseOutlinerColor()

                outlinerColor = self.item.getGroupMetadata(self.GROUP, self.OUTLINER_COLOR)
                if not outlinerColor.empty() and (outlinerColor.typeName() == "ufe.Vector3d"):
                    # Color is stored as double3 USD custom data.
                    clr = ufe.Vector3d(outlinerColor)
                    cmds.colorSliderGrp(self.outlinerColor, edit=True,
                                        rgb=(clr.x(), clr.y(), clr.z()))
                else:
                    self._clearOutlinerColor()
            else:
                # Get the custom data directly from USD.
                useOutlinerColor = self.prim.GetCustomDataByKey(self.USE_OUTLINER_COLOR)
                if useOutlinerColor is not None and isinstance(useOutlinerColor, bool):
                    cmds.checkBoxGrp(self.useOutlinerColor, edit=True, v1=useOutlinerColor)
                else:
                    self._clearUseOutlinerColor()

                outlinerColor = self.prim.GetCustomDataByKey(self.OUTLINER_COLOR)
                if outlinerColor is not None and isinstance(outlinerColor, Gf.Vec3d):
                    # Color is stored as double3 USD custom data.
                    cmds.colorSliderGrp(self.outlinerColor, edit=True,
                                        rgb=(outlinerColor[0], outlinerColor[1], outlinerColor[2]))
                else:
                    self._clearOutlinerColor()
        except:
            self.clear()

    def _updateTextColorChanged(self):
        '''Update the text color custom data for this prim based on the values
        set in the two fields.'''
        # Get the value of "Use Outliner Color" checkbox.
        useTextColor = cmds.checkBoxGrp(self.useOutlinerColor, query=True, v1=True)
        # Get the value of "Outliner Color" color slider.
        rgb = cmds.colorSliderGrp(self.outlinerColor, query=True, rgbValue=True)
        try:
            if self.useMetadata:
                # Get ufe commands for the two metadata.
                cmd1 = self.item.setGroupMetadataCmd(self.GROUP, self.USE_OUTLINER_COLOR, useTextColor)
                ufeVec = ufe.Vector3d(rgb[0], rgb[1], rgb[2])
                cmd2 = self.item.setGroupMetadataCmd(self.GROUP, self.OUTLINER_COLOR, ufe.Value(ufeVec))

                # Create ufe composite command from both commands above and execute it.
                cmd = ufe.CompositeUndoableCommand([cmd1, cmd2])
                ufeCmd.execute(cmd)
            else:
                with mayaUsdLib.UsdUndoBlock():
                    # As initially decided write out the color custom data to the session layer.
                    # It still can be edit-routed as a 'primMetadata' operation.
                    fallbackLayer = self.prim.GetStage().GetSessionLayer()
                    with PrimCustomDataEditRouting(self.prim, self.USE_OUTLINER_COLOR, fallbackLayer):
                        self.prim.SetCustomDataByKey(self.USE_OUTLINER_COLOR, useTextColor)
                    with PrimCustomDataEditRouting(self.prim, self.OUTLINER_COLOR, fallbackLayer):
                        self.prim.SetCustomDataByKey(self.OUTLINER_COLOR, Gf.Vec3d(rgb[0], rgb[1], rgb[2]))
        except Exception as ex:
            # Note: the command might not work because there is a stronger
            #       opinion or an editRouting prevention so update the metadata controls.
            self.refresh()
            cmds.error(str(ex))

    def _onUseOutlinerColorChanged(self, value):
        self._updateTextColorChanged()

    def _onOutlinerColorChanged(self, value):
        self._updateTextColorChanged()
