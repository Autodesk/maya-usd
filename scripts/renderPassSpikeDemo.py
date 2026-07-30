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
"""Demo helper for the USD render pass viewport filter spike.

Authors a USD stage with three colour-coded groups and a set of render passes
that filter them in different ways, loads it into a new proxy shape, and opens a
window for switching the proxy shape's activeRenderPass attribute.

Run inside Maya:

    import renderPassSpikeDemo
    renderPassSpikeDemo.show()

The scene is nine cubes in three groups of three, laid out left to right:

    RED (x=-4)        GREEN (x=0)       BLUE (x=+4)

Each pass below states what should disappear. Because prune and renderVisibility
look identical on screen, the passes are arranged so that each one leaves a
different set of groups standing -- if the wrong group vanishes, or nothing
does, the filter is misbehaving.
"""

import os
import tempfile

from maya import cmds

from pxr import Gf, Sdf, Usd, UsdGeom, UsdRender

WINDOW_NAME = 'renderPassSpikeDemoWindow'

GROUPS = [
    ('RedGroup', -4.0, (1.0, 0.1, 0.1)),
    ('GreenGroup', 0.0, (0.1, 1.0, 0.1)),
    ('BlueGroup', 4.0, (0.1, 0.3, 1.0)),
]

# (pass name, prune expression, renderVisibility expression, what you should see)
PASSES = [
    ('NoFilter', '', '', 'Control: all nine cubes visible.'),
    ('PruneRed', '/World/RedGroup//', '', 'RED gone (pruned). Green + blue remain.'),
    ('HideBlue', '', '/World/RedGroup// + /World/GreenGroup//',
     'BLUE gone (made invisible). Red + green remain.'),
    ('OnlyGreen', '', '/World/GreenGroup//',
     'Only GREEN remains; red and blue are made invisible.'),
    ('PruneRedHideBlue', '/World/RedGroup//', '/World/RedGroup// + /World/GreenGroup//',
     'Only GREEN remains: red is pruned, blue is made invisible.'),
]


def _addCubeGroup(stage, name, xOffset, color):
    group = UsdGeom.Xform.Define(stage, '/World/{}'.format(name))
    group.AddTranslateOp().Set(Gf.Vec3d(xOffset, 0.0, 0.0))

    for i in range(3):
        cube = UsdGeom.Cube.Define(stage, '/World/{}/Cube_{}'.format(name, i + 1))
        cube.CreateSizeAttr(1.5)
        UsdGeom.Xformable(cube.GetPrim()).AddTranslateOp().Set(Gf.Vec3d(0.0, i * 2.0, 0.0))
        cube.CreateDisplayColorAttr([Gf.Vec3f(*color)])


def _addRenderPass(stage, name, pruneExpr, renderVisExpr):
    renderPass = UsdRender.Pass.Define(stage, '/Render/Passes/{}'.format(name))
    prim = renderPass.GetPrim()

    # Only the membershipExpression form of a collection is carried by
    # HdCollectionSchema, so includes/excludes would be ignored by the filter.
    if pruneExpr:
        collection = Usd.CollectionAPI.Apply(prim, 'prune')
        collection.CreateMembershipExpressionAttr(Sdf.PathExpression(pruneExpr))

    if renderVisExpr:
        collection = Usd.CollectionAPI.Apply(prim, 'renderVisibility')
        collection.CreateMembershipExpressionAttr(Sdf.PathExpression(renderVisExpr))


def buildStage(filePath=None):
    """Author the demo stage and return the file path it was written to."""
    if filePath is None:
        filePath = os.path.join(tempfile.gettempdir(), 'renderPassSpikeDemo.usda')

    stage = Usd.Stage.CreateInMemory()

    UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)
    UsdGeom.Xform.Define(stage, '/World')
    stage.SetDefaultPrim(stage.GetPrimAtPath('/World'))

    for name, xOffset, color in GROUPS:
        _addCubeGroup(stage, name, xOffset, color)

    UsdGeom.Scope.Define(stage, '/Render')
    UsdGeom.Scope.Define(stage, '/Render/Passes')
    for name, pruneExpr, renderVisExpr, _ in PASSES:
        _addRenderPass(stage, name, pruneExpr, renderVisExpr)

    stage.GetRootLayer().Export(filePath)
    return filePath


def createProxy(filePath):
    """Create a proxy shape in a fresh scene loading filePath. Returns its name."""
    cmds.file(new=True, force=True)
    cmds.upAxis(axis='y')

    cmds.createNode('mayaUsdProxyShape', name='renderPassSpikeShape')
    shapeNode = cmds.ls(selection=True, long=True)[0]
    cmds.setAttr('{}.filePath'.format(shapeNode), filePath, type='string')
    cmds.connectAttr('time1.outTime', '{}.time'.format(shapeNode))
    cmds.select(clear=True)

    cmds.viewFit(all=True)
    return shapeNode


def _setActivePass(shapeNode, passName, statusField):
    passPath = '/Render/Passes/{}'.format(passName) if passName else ''
    cmds.setAttr('{}.activeRenderPass'.format(shapeNode), passPath, type='string')

    expected = next((p[3] for p in PASSES if p[0] == passName), '')
    cmds.text(statusField, edit=True, label='{}  --  {}'.format(passPath or '(none)', expected))
    cmds.refresh(force=True)


def _showWindow(shapeNode):
    if cmds.window(WINDOW_NAME, exists=True):
        cmds.deleteUI(WINDOW_NAME)

    cmds.window(WINDOW_NAME, title='Render Pass Spike', widthHeight=(520, 260))
    cmds.columnLayout(adjustableColumn=True, rowSpacing=6, columnOffset=('both', 10))

    cmds.text(label='Proxy shape: {}'.format(shapeNode), align='left')
    cmds.separator(height=8, style='in')

    statusField = cmds.text(label='(none)', align='left', wordWrap=True, height=40)

    cmds.button(
        label='No active pass (clear)',
        command=lambda *_: _setActivePass(shapeNode, '', statusField))

    for name, _, _, expected in PASSES:
        cmds.button(
            label='{} -- {}'.format(name, expected),
            align='left',
            command=lambda *_, n=name: _setActivePass(shapeNode, n, statusField))

    cmds.showWindow(WINDOW_NAME)


def checkReprefix():
    """Sanity-check the path-expression rebasing the C++ side depends on.

    UsdImagingDelegate prefixes every prim it emits with its delegate ID, so
    collection expressions authored against raw USD paths are rebased onto that
    prefix before use. If ReplacePrefix does not handle the `//` descendant
    patterns these passes use, nothing matches and the viewport silently shows
    no filtering at all -- which is indistinguishable from the filter never
    being reached. Run this first to tell the two apart.
    """
    prefix = Sdf.Path('/Proxy_test')
    ok = True

    if not hasattr(Sdf.PathExpression, 'ReplacePrefix'):
        print('Reprefix check: SKIPPED -- SdfPathExpression.ReplacePrefix is not exposed to '
              'Python. The C++ side still uses it; this check just cannot verify it here.')
        return True

    for name, pruneExpr, renderVisExpr, _ in PASSES:
        for label, exprStr in (('prune', pruneExpr), ('renderVisibility', renderVisExpr)):
            if not exprStr:
                continue
            rebased = Sdf.PathExpression(exprStr).ReplacePrefix(Sdf.Path.absoluteRootPath, prefix)
            matched = str(rebased).count(str(prefix))
            expected = str(exprStr).count('/World')
            status = 'OK ' if matched == expected else 'BAD'
            if matched != expected:
                ok = False
            print('  {} {}/{}: {!r} -> {!r}'.format(status, name, label, exprStr, str(rebased)))

    print('Reprefix check: {}'.format('PASSED' if ok else 'FAILED -- expressions will match '
                                      'nothing and no filtering will be visible'))
    return ok


def enableDebugOutput(enable=True):
    """Turn on HDVP2_DEBUG_RENDER_PASS tracing.

    Must happen before the proxy shape first draws, since the scene index chain
    is built during render index construction.
    """
    from pxr import Tf
    Tf.Debug.SetDebugSymbolsByName('HDVP2_DEBUG_RENDER_PASS', enable)


def show(filePath=None, debug=True):
    """Author the stage, load it into a new proxy shape, and open the UI."""
    enableDebugOutput(debug)
    checkReprefix()
    usdFile = buildStage(filePath)
    shapeNode = createProxy(usdFile)
    _showWindow(shapeNode)
    print('Render pass spike demo stage: {}'.format(usdFile))
    return shapeNode
    
show()
