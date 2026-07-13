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
shared layer_editor_test.py suite can run in both DCCs.
"""

from maya import cmds

import mayaUsd
import mayaUsd_createStageWithNewLayer

from layer_editor_test import UsdLayerEditorTest


# Per-test undo/redo stacks for UsdLayerEditor commands.  resetScene() clears
# them so each test starts with a clean slate.
_undo_stack = []
_redo_stack = []


def _newScene():
    """Reset the Maya scene and ensure the mayaUsd plugin is loaded."""
    # Close any Layer Editor left open by a previous test so each test starts with a
    # clean slate. A leaked editor keeps a live model observing stage changes, whose
    # deferred (idle) system-lock refresh could otherwise fire during an unrelated test.
    if cmds.window('mayaUsdLayerEditor', exists=True):
        cmds.deleteUI('mayaUsdLayerEditor', window=True)
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
    # Force Maya to evaluate the proxy shape so the stage and its full layer
    # stack are populated before the test accesses them.
    cmds.refresh(force=True)
    stage = _stageFromShape(shapeNode)
    cmds.select(clear=True)
    cmds.connectAttr('time1.outTime', '{}.time'.format(shapeNode))
    # Second refresh so UsdStageMap registers the wired proxy shape; without
    # this, UsdUfe::stagePath(stage) returns an empty path and
    # usdUfe.getStage(objectPath) returns None inside onRefreshSystemLock
    # callbacks.
    cmds.refresh(force=True)
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
        # Reload from disk so any in-memory modifications from a previous stage
        # that shared the same SdfLayer (same file path) don't carry over.
        stage.Reload()
    else:
        shapePath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = _stageFromShape(shapePath)
    return stage


def resetScene():
    _undo_stack.clear()
    _redo_stack.clear()
    _newScene()


def undo():
    if _undo_stack:
        cmd = _undo_stack.pop()
        cmd.undo()
        _redo_stack.append(cmd)


def redo():
    if _redo_stack:
        cmd = _redo_stack.pop()
        cmd.redo()
        _undo_stack.append(cmd)


def executeCmd(cmd):
    """Execute a UsdLayerEditor command and push it onto the local undo stack.

    UsdLayerEditor commands are Boost.Python-bound and cannot be passed to
    ufe.UndoableCommandMgr (pybind11).  Since the commands expose undo()/redo()
    directly, we manage the stack ourselves rather than going through Maya's MEL
    undo machinery.
    """
    cmd.execute()
    _undo_stack.append(cmd)
    _redo_stack.clear()
    # Process pending Qt timer events (e.g. async layer-tree model rebuilds)
    # so the UI is up to date before the test makes assertions.
    if cmds.window('mayaUsdLayerEditor', exists=True):
        cmds.refresh(force=True)


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
    # usdUfe.getStage() expects a path without the |world prefix, but
    # Ufe::Path::string() (used in C++ callbacks) includes it. Patch getStage
    # so callbacks that receive objectPath from C++ work correctly in Maya.
    import usdUfe
    _orig_getStage = usdUfe.getStage

    def _maya_getStage(objectPath):
        if objectPath and objectPath.startswith('|world'):
            objectPath = objectPath[len('|world'):]
        return _orig_getStage(objectPath)

    usdUfe.getStage = _maya_getStage

    UsdLayerEditorTest._createStage = staticmethod(createStage)
    UsdLayerEditorTest._resetScene = staticmethod(resetScene)
    UsdLayerEditorTest._undo = staticmethod(undo)
    UsdLayerEditorTest._redo = staticmethod(redo)
    UsdLayerEditorTest._openStageLayerEditor = staticmethod(openStageLayerEditor)
    UsdLayerEditorTest._executeCmd = staticmethod(executeCmd)
