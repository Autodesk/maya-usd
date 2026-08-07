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

Authors a USD stage, loads it into a new proxy shape, creates a shadow-casting
light, configures the viewport for shaded display with shadows, and opens a
window for switching the scene's active render pass.

The active pass is scene-wide, held on the UsdDefaultRenderSettings singleton as
a UFE path ("<proxy shape>,<prim path>"). Each proxy shape filters only when the
gateway segment of that path names it, so at most one stage is ever filtered.

Run inside Maya:

    import renderPassSpikeDemo
    renderPassSpikeDemo.show()

Scene: twelve cubes in four groups of three on a ground plane.

    RED (x=-4)   GREEN (x=0)   BLUE (x=+4)   SHADED (x=+8)

The first three carry only displayColor. SHADED has a real UsdPreviewSurface
bound, which is what makes it useful: the matte flag colour has to beat a bound
material, and a prim with no material would not prove that. The ground plane
exists to catch shadows.

What each collection should look like:

  prune             geometry disappears, and leaves the Hydra scene entirely
  renderVisibility  geometry disappears, shadow disappears with it
  matte             geometry turns flat magenta
  cameraVisibility  geometry disappears BUT ITS SHADOW REMAINS
  camera            nothing on switching; use the Look through pass camera
                    button, which resolves renderSource -> settings -> camera

The cameraVisibility cases are only meaningful with shadows on, which is why
show() sets the viewport up rather than leaving it to you. Compare CamHideRed
against VisHideRed: both hide the red cubes, and only the renderVisibility one
should also lose the shadow. If they look identical, cameraVisibility is not
working even though the viewport looks plausible.

For every collection, the case that matters is switching *away* from a pass.
Making something disappear or turn magenta only proves half the path; if it
does not come back on NoFilter, the invalidation is broken.
"""

import os
import tempfile

from maya import cmds

from mayaUsd import lib as mayaUsdLib
from mayaUsd.lib import UsdDefaultRenderSettings

from pxr import Gf, Sdf, Usd, UsdGeom, UsdRender, UsdShade

WINDOW_NAME = 'renderPassSpikeDemoWindow'

GROUPS = [
    ('RedGroup', -4.0, (1.0, 0.1, 0.1)),
    ('GreenGroup', 0.0, (0.1, 1.0, 0.1)),
    ('BlueGroup', 4.0, (0.1, 0.3, 1.0)),
]

_ALL = ['/World/RedGroup//', '/World/GreenGroup//', '/World/BlueGroup//',
        '/World/ShadedGroup//', '/World/Ground']


def _allExcept(*excluded):
    """Build an include-style expression covering everything but `excluded`."""
    return ' + '.join(p for p in _ALL if p not in excluded)


# Each pass is a dict so cases can be added without re-threading a tuple.
# Keys: name, prune, renderVisibility, matte, cameraVisibility, expect.
PASSES = [
    dict(name='NoFilter',
         expect='Control: all twelve cubes visible, none magenta.'),

    dict(name='PruneRed', prune='/World/RedGroup//',
         expect='RED gone (pruned), shadow gone too.'),

    dict(name='OnlyGreen', renderVisibility='/World/GreenGroup// + /World/Ground',
         expect='Only GREEN remains; red, blue and shaded made invisible.'),

    # Matte
    dict(name='MatteGreen', matte='/World/GreenGroup//',
         expect='All visible; GREEN turns MAGENTA.'),

    dict(name='MatteShaded', matte='/World/ShadedGroup//',
         expect='SHADED (grey, lit) turns MAGENTA -- proves matte beats a material.'),

    dict(name='MatteAll', matte='/World//',
         expect='Every cube turns MAGENTA, shaded group included.'),

    # cameraVisibility. Polarity matches renderVisibility: prims NOT matching
    # the collection are the ones hidden from camera.
    dict(name='CamHideRed', cameraVisibility=_allExcept('/World/RedGroup//'),
         expect='RED gone BUT ITS SHADOW REMAINS on the ground.'),

    dict(name='VisHideRed', renderVisibility=_allExcept('/World/RedGroup//'),
         expect='A/B control: RED gone AND its shadow gone. Compare with CamHideRed.'),

    dict(name='CamHideShaded', cameraVisibility=_allExcept('/World/ShadedGroup//'),
         expect='SHADED gone, its shadow remains.'),

    dict(name='CamHideRedMatteGreen',
         cameraVisibility=_allExcept('/World/RedGroup//'),
         matte='/World/GreenGroup//',
         expect='RED gone (shadow remains), GREEN magenta.'),

    # These author renderSource -> UsdRenderSettings, which is where the camera
    # lives. Switching to them changes nothing on its own by design; the
    # Look through pass camera button is the only thing that moves the viewport.
    dict(name='CamShotRed', camera='/Cameras/PassCamRed',
         expect='Look through pass camera frames the RED group.'),

    dict(name='CamShotShaded', camera='/Cameras/PassCamShaded',
         expect='Look through pass camera frames the SHADED group.'),

    dict(name='CamShotWide', camera='/Cameras/PassCamWide',
         expect='Look through pass camera frames the whole set.'),
]

COLLECTIONS = ('prune', 'renderVisibility', 'matte', 'cameraVisibility')

# (name, eye, look-at target)
CAMERAS = [
    ('PassCamRed', (-4.0, 6.0, 16.0), (-4.0, 2.0, 0.0)),
    ('PassCamShaded', (8.0, 6.0, 16.0), (8.0, 2.0, 0.0)),
    ('PassCamWide', (2.0, 14.0, 34.0), (0.0, 2.0, 0.0)),
]


def _addCubeGroup(stage, name, xOffset, color):
    group = UsdGeom.Xform.Define(stage, '/World/{}'.format(name))
    group.AddTranslateOp().Set(Gf.Vec3d(xOffset, 0.0, 0.0))

    for i in range(3):
        cube = UsdGeom.Cube.Define(stage, '/World/{}/Cube_{}'.format(name, i + 1))
        cube.CreateSizeAttr(1.5)
        UsdGeom.Xformable(cube.GetPrim()).AddTranslateOp().Set(Gf.Vec3d(0.0, i * 2.0, 0.0))
        cube.CreateDisplayColorAttr([Gf.Vec3f(*color)])


def _addShadedGroup(stage, name, xOffset):
    """A group with a real UsdPreviewSurface bound, so matte can be shown beating it."""
    material = UsdShade.Material.Define(stage, '/World/Materials/{}Mat'.format(name))
    shader = UsdShade.Shader.Define(stage, '/World/Materials/{}Mat/Surface'.format(name))
    shader.CreateIdAttr('UsdPreviewSurface')
    shader.CreateInput('diffuseColor', Sdf.ValueTypeNames.Color3f).Set(Gf.Vec3f(0.55, 0.55, 0.6))
    shader.CreateInput('roughness', Sdf.ValueTypeNames.Float).Set(0.4)
    shader.CreateInput('metallic', Sdf.ValueTypeNames.Float).Set(0.0)
    material.CreateSurfaceOutput().ConnectToSource(shader.ConnectableAPI(), 'surface')

    group = UsdGeom.Xform.Define(stage, '/World/{}'.format(name))
    group.AddTranslateOp().Set(Gf.Vec3d(xOffset, 0.0, 0.0))

    for i in range(3):
        cube = UsdGeom.Cube.Define(stage, '/World/{}/Cube_{}'.format(name, i + 1))
        cube.CreateSizeAttr(1.5)
        UsdGeom.Xformable(cube.GetPrim()).AddTranslateOp().Set(Gf.Vec3d(0.0, i * 2.0, 0.0))
        UsdShade.MaterialBindingAPI.Apply(cube.GetPrim()).Bind(material)


def _addGround(stage):
    """A quad for shadows to land on. Without it the cameraVisibility cases prove nothing."""
    extent = 24.0
    mesh = UsdGeom.Mesh.Define(stage, '/World/Ground')
    mesh.CreatePointsAttr([
        Gf.Vec3f(-extent, 0.0, -extent), Gf.Vec3f(extent, 0.0, -extent),
        Gf.Vec3f(extent, 0.0, extent), Gf.Vec3f(-extent, 0.0, extent)])
    mesh.CreateFaceVertexCountsAttr([4])
    mesh.CreateFaceVertexIndicesAttr([0, 1, 2, 3])
    mesh.CreateNormalsAttr([Gf.Vec3f(0.0, 1.0, 0.0)] * 4)
    mesh.SetNormalsInterpolation(UsdGeom.Tokens.vertex)
    mesh.CreateSubdivisionSchemeAttr(UsdGeom.Tokens.none)
    mesh.CreateExtentAttr([Gf.Vec3f(-extent, 0.0, -extent), Gf.Vec3f(extent, 0.0, extent)])
    mesh.CreateDisplayColorAttr([Gf.Vec3f(0.4, 0.4, 0.42)])
    UsdGeom.Xformable(mesh.GetPrim()).AddTranslateOp().Set(Gf.Vec3d(2.0, -1.0, 0.0))


def _addCameras(stage):
    for name, eye, target in CAMERAS:
        camera = UsdGeom.Camera.Define(stage, '/Cameras/{}'.format(name))
        camera.CreateFocalLengthAttr(35.0)
        # USD cameras look down -Z, so the transform is the inverse of the view.
        view = Gf.Matrix4d().SetLookAt(
            Gf.Vec3d(*eye), Gf.Vec3d(*target), Gf.Vec3d(0.0, 1.0, 0.0))
        UsdGeom.Xformable(camera.GetPrim()).AddTransformOp().Set(view.GetInverse())


def _addRenderPass(stage, spec):
    renderPass = UsdRender.Pass.Define(stage, '/Render/Passes/{}'.format(spec['name']))
    prim = renderPass.GetPrim()

    # Only the membershipExpression form of a collection is carried by
    # HdCollectionSchema, so includes/excludes would be ignored by the filter.
    for collectionName in COLLECTIONS:
        expr = spec.get(collectionName, '')
        if expr:
            collection = Usd.CollectionAPI.Apply(prim, collectionName)
            collection.CreateMembershipExpressionAttr(Sdf.PathExpression(expr))

    # The camera lives on a RenderSettings prim reached through renderSource,
    # not on the pass itself.
    camera = spec.get('camera', '')
    if not camera:
        return

    settings = UsdRender.Settings.Define(stage, '/Render/Settings/{}'.format(spec['name']))
    settings.CreateCameraRel().SetTargets([Sdf.Path(camera)])
    renderPass.CreateRenderSourceRel().SetTargets([settings.GetPrim().GetPath()])


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
    _addShadedGroup(stage, 'ShadedGroup', 8.0)
    _addGround(stage)
    _addCameras(stage)

    UsdGeom.Scope.Define(stage, '/Render')
    UsdGeom.Scope.Define(stage, '/Render/Passes')
    for spec in PASSES:
        _addRenderPass(stage, spec)

    stage.GetRootLayer().Export(filePath)
    return filePath


def createLight():
    """A directional light with depth map shadows, angled to throw shadows sideways."""
    lightShape = cmds.directionalLight(intensity=1.4)
    lightTransform = cmds.listRelatives(lightShape, parent=True, fullPath=True)[0]
    cmds.setAttr('{}.rotateX'.format(lightTransform), -40.0)
    cmds.setAttr('{}.rotateY'.format(lightTransform), -30.0)
    cmds.setAttr('{}.useDepthMapShadows'.format(lightShape), 1)
    cmds.setAttr('{}.dmapResolution'.format(lightShape), 2048)
    cmds.setAttr('{}.dmapFilterSize'.format(lightShape), 2)
    return lightShape


def setupViewport():
    """Put every model panel into shaded mode with lighting and shadows on."""
    panels = cmds.getPanel(type='modelPanel') or []
    for panel in panels:
        try:
            cmds.modelEditor(
                panel, edit=True,
                displayAppearance='smoothShaded',
                displayLights='all',
                shadows=True,
                wireframeOnShaded=False)
        except RuntimeError:
            # Some panels are not editable in every session state; skip them.
            pass
    return panels


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


def _activePassPath(shapeNode):
    """USD path of the active render pass, if this proxy shape owns it."""
    activePath = UsdDefaultRenderSettings.getActiveRenderSettingsPath()
    prefix = '{},'.format(shapeNode)
    return activePath[len(prefix):] if activePath.startswith(prefix) else ''


def _setActivePass(shapeNode, passName, statusField):
    passPath = '/Render/Passes/{}'.format(passName) if passName else ''
    # Scene-wide, not per proxy shape: the UFE path names the owning shape.
    ok = UsdDefaultRenderSettings.setActiveRenderSettingsPath(
        '{},{}'.format(shapeNode, passPath) if passPath else '')
    if not ok:
        # Returns False when the singleton is missing, which looks exactly like
        # a filter that ran and matched nothing.
        cmds.text(statusField, edit=True,
                  label='FAILED to set active pass -- no UsdDefaultRenderSettings node.')
        return

    expected = next((p['expect'] for p in PASSES if p['name'] == passName), '')
    cmds.text(statusField, edit=True, label='{}  --  {}'.format(passPath or '(none)', expected))
    cmds.refresh(force=True)


def lookThroughPassCamera(shapeNode, statusField=None):
    """Look through the active pass's camera. Explicit only, never automatic.

    Needs no C++: ProxyShapeCameraHandler already exposes USD cameras to UFE and
    cmds.lookThru accepts a UFE path of the form "<shape dag path>,<prim path>".
    """
    def report(message):
        if statusField:
            cmds.text(statusField, edit=True, label=message)
        print(message)

    passPath = _activePassPath(shapeNode)
    if not passPath:
        report('No active render pass on {}.'.format(shapeNode))
        return None

    stage = mayaUsdLib.GetPrim(shapeNode).GetStage()
    renderPass = UsdRender.Pass(stage.GetPrimAtPath(passPath))
    if not renderPass:
        report('No render pass at {}.'.format(passPath))
        return None

    sources = renderPass.GetRenderSourceRel().GetTargets()
    if not sources:
        report('{} has no renderSource, so no camera.'.format(passPath))
        return None

    settings = UsdRender.Settings(stage.GetPrimAtPath(sources[0]))
    cameras = settings.GetCameraRel().GetTargets() if settings else []
    if not cameras:
        report('{} resolves no camera.'.format(passPath))
        return None

    if not UsdGeom.Camera(stage.GetPrimAtPath(cameras[0])):
        report('{} is not a UsdGeomCamera.'.format(cameras[0]))
        return None

    ufePath = '{},{}'.format(shapeNode, cameras[0])
    cmds.lookThru(ufePath)
    report('Looking through {}'.format(ufePath))
    return ufePath


def _showWindow(shapeNode):
    if cmds.window(WINDOW_NAME, exists=True):
        cmds.deleteUI(WINDOW_NAME)

    cmds.window(WINDOW_NAME, title='Render Pass Spike', widthHeight=(640, 420))
    cmds.columnLayout(adjustableColumn=True, rowSpacing=6, columnOffset=('both', 10))

    cmds.text(label='Proxy shape: {}'.format(shapeNode), align='left')
    cmds.separator(height=8, style='in')

    statusField = cmds.text(label='(none)', align='left', wordWrap=True, height=40)

    cmds.button(
        label='No active pass (clear)',
        command=lambda *_: _setActivePass(shapeNode, '', statusField))

    cmds.button(
        label='Look through pass camera',
        command=lambda *_: lookThroughPassCamera(shapeNode, statusField))

    cmds.separator(height=8, style='in')

    for spec in PASSES:
        cmds.button(
            label='{} -- {}'.format(spec['name'], spec['expect']),
            align='left',
            command=lambda *_, n=spec['name']: _setActivePass(shapeNode, n, statusField))

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

    for spec in PASSES:
        for collectionName in COLLECTIONS:
            exprStr = spec.get(collectionName, '')
            if not exprStr:
                continue
            rebased = Sdf.PathExpression(exprStr).ReplacePrefix(Sdf.Path.absoluteRootPath, prefix)
            matched = str(rebased).count(str(prefix))
            expected = str(exprStr).count('/World')
            status = 'OK ' if matched == expected else 'BAD'
            if matched != expected:
                ok = False
            print('  {} {}/{}: {!r} -> {!r}'.format(
                status, spec['name'], collectionName, exprStr, str(rebased)))

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
    """Author the stage, load it, light it, set up shadows, and open the UI."""
    enableDebugOutput(debug)
    checkReprefix()

    usdFile = buildStage(filePath)
    shapeNode = createProxy(usdFile)
    createLight()
    setupViewport()
    cmds.viewFit(all=True)

    _showWindow(shapeNode)
    print('Render pass spike demo stage: {}'.format(usdFile))
    print('Shadows are enabled in all model panels. If you see no shadows at all, '
          'the cameraVisibility cases cannot be judged.')
    return shapeNode


show()