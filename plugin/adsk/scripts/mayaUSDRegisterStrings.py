# Copyright 2021 Autodesk
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
import maya.mel as mel
from mayaUsdLibRegisterStrings import registerPluginResource, getPluginResource

def register(key, value):
    registerPluginResource('mayaUsdPlugin', key, value)

def getMayaUsdString(key):
    return getPluginResource('mayaUsdPlugin', key)

def mayaUSDRegisterStrings():
    # This function is called from the equivalent MEL proc
    # with the same name. The strings registered here and all the
    # ones registered from the MEL proc can be used in either
    # MEL or python.
    register("kButtonYes", "Yes")
    register("kButtonNo", "No")
    register("kDiscardStageEditsTitle", "Discard Edits on ^1s's Layers")
    register("kDiscardStageEditsLoadMsg", "Are you sure you want to load in a new file as the stage source?\n\nAll edits on your layers in ^1s will be discarded.")
    register("kDiscardStageEditsReloadMsg", "Are you sure you want to reload ^1s as the stage source?\n\nAll edits on your layers (except the session layer) in ^2s will be discarded.")
    register("kLoadUSDFile", "Load USD File")
    register("kFileOptions", "File Options")
    register("kRelativePathOptions", "Relative Pathing")
    register("kMakePathRelativeToSceneFile", "Make Path Relative to Scene File")
    register("kMakePathRelativeToSceneFileAnn", "Path will be relative to your Maya scene file.")
    register("kMakePathRelativeToEditTargetLayer", "Make Path Relative to Edit Target Layer Directory")
    register("kMakePathRelativeToEditTargetLayerAnn", "Enable to activate relative pathing to your current edit target layer's directory.\nIf this option is disabled, verify that your target layer is not anonymous and save it to disk.")
    register("kMakePathRelativeToParentLayer", "Make Path Relative to Parent Layer Directory")
    register("kMakePathRelativeToParentLayerAnn", "Enable to activate relative pathing to your current parent layer's directory.\nIf this option is disabled, verify that your parent layer is not anonymous and save it to disk.")
    register("kUnresolvedPath", "Path Preview:")
    register("kUnresolvedPathAnn", "This field indicates the path with the file name currently chosen in your text input. Note: This is the string that will be written out to the file in the chosen directory in order to enable portability.")
    register("kCompositionArcOptions", "Composition Arc Options")
    register("kPreview", "Preview")
