#
# Copyright 2026 Autodesk
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
"""Bind the DCC-agnostic UsdLayerEditorTest static hooks to Maya implementations.

Mirrors the 3dsmax setup script (max_layer_editor_test_setup.py) so the same
shared layer_editor_test.py suite can run in both DCCs. Tasks 7-9 of the
USD Layer Editor migration plan.
"""

from maya import cmds

import mayaUsd
import mayaUsd_createStageWithNewLayer

from layer_editor_test import UsdLayerEditorTest


def _newScene():
    """Reset the Maya scene and ensure the mayaUsd plugin is loaded."""
    cmds.file(new=True, force=True)
    if not cmds.pluginInfo('mayaUsdPlugin', query=True, loaded=True):
        cmds.loadPlugin('mayaUsdPlugin')


def _stageFromShape(shapePath):
    return mayaUsd.lib.GetPrim(shapePath).GetStage()


def _createProxyFromFile(filePath):
    """Create a mayaUsdProxyShape pointing at filePath; return (shapePath, stage)."""
    cmds.createNode('mayaUsdProxyShape', name='stageShape')
    shapeNode = cmds.ls(sl=True, l=True)[0]
    cmds.setAttr('{}.filePath'.format(shapeNode), filePath, type='string')
    stage = _stageFromShape(shapeNode)
    cmds.select(clear=True)
    cmds.connectAttr('time1.outTime', '{}.time'.format(shapeNode))
    return shapeNode, stage


def createStage(rootFile):
    """Create a Maya USD stage backed by ``rootFile`` and return its ``Usd.Stage``.

    If ``rootFile`` is empty, a stage with a fresh anonymous root layer is
    returned. Otherwise a ``mayaUsdProxyShape`` is created pointing at the
    given USD file path.
    """
    _newScene()
    if rootFile:
        _shapePath, stage = _createProxyFromFile(rootFile)
    else:
        shapePath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = _stageFromShape(shapePath)
    return stage


def resetScene():
    _newScene()


def undo():
    cmds.undo()


def redo():
    cmds.redo()


def executeCmd(cmd):
    """Execute a UsdLayerEditor command in Maya.

    UsdLayerEditor's command classes are bound with boost.python while UFE is
    bound with pybind11, so the commands can't cross into
    ``ufe.UndoableCommandMgr.executeCmd()``. Wrap a direct ``cmd.execute()``
    in a Maya undo chunk so the operation participates in MEL undo via
    whatever MEL/MPx commands the C++ implementation issues.
    """
    cmds.undoInfo(openChunk=True)
    try:
        cmd.execute()
    finally:
        cmds.undoInfo(closeChunk=True)


def openStageLayerEditor(rootFile):
    """Create a Maya USD stage from ``rootFile`` and open the Layer Editor on it.

    Returns the ``Usd.Stage`` so the shared test can interact with it.
    """
    _newScene()
    if rootFile:
        shapePath, stage = _createProxyFromFile(rootFile)
    else:
        shapePath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = _stageFromShape(shapePath)

    # Show the session layer (some shared tests rely on it being visible).
    cmds.optionVar(intValue=('MayaUSDLayerEditor_AutoHideSessionLayer', 0))

    # Open the Layer Editor window on the proxy shape.
    cmds.mayaUsdLayerEditorWindow('mayaUsdLayerEditor', proxyShape=shapePath)
    cmds.refresh(force=True)

    return stage


def setup():
    """Install the Maya implementations into ``UsdLayerEditorTest``."""
    UsdLayerEditorTest._createStage = staticmethod(createStage)
    UsdLayerEditorTest._resetScene = staticmethod(resetScene)
    UsdLayerEditorTest._undo = staticmethod(undo)
    UsdLayerEditorTest._redo = staticmethod(redo)
    UsdLayerEditorTest._openStageLayerEditor = staticmethod(openStageLayerEditor)
    UsdLayerEditorTest._executeCmd = staticmethod(executeCmd)
