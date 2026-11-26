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

"""
Helpers shared by the Gaussian splat render delegate tests: scene building,
snapshot measurements (coverage, dominant colors, ellipse axes), Maya log
capture, selection and picking, and baked twin comparison for instancing.

Scenes are built in code from small splat prims, and the splat output is
stochastic but repeatable, so tests measure the 960x540 snapshot instead of
comparing it to baseline images.
"""

import ctypes
import math
import os

import numpy as np

from maya import cmds
import maya.api.OpenMaya as om
import maya.api.OpenMayaUI as omui

import mayaUtils
import imageUtils
import usdUtils

import ufe

from mayaUsd import lib as mayaUsdLib

from pxr import Gf, Sdf, Usd, UsdGeom, Vt

try:
    from pxr import UsdVol
except ImportError:
    UsdVol = None

try:
    from PySide2 import QtCore, QtGui
    from PySide2.QtWidgets import QWidget
    from shiboken2 import wrapInstance
except ImportError:
    from PySide6 import QtCore, QtGui
    from PySide6.QtWidgets import QWidget
    from shiboken6 import wrapInstance

import qtInputHelpers

SNAPSHOT_WIDTH = 960
SNAPSHOT_HEIGHT = 540

SPLAT_SCHEMA_NAME = 'ParticleField3DGaussianSplat'

# Attribute names as authored on the splat prim (from the UsdVol schema).
ATTR_POSITIONS = 'positions'
ATTR_OPACITIES = 'opacities'
ATTR_SCALES = 'scales'
ATTR_ORIENTATIONS = 'orientations'
ATTR_SH_COEFFS = 'radiance:sphericalHarmonicsCoefficients'
ATTR_SH_DEGREE = 'radiance:sphericalHarmonicsDegree'


# The semi-axes of a splat without authored scales, and the fraction of the quad half-axes
# per unit of scale (the ellipse of a splat has semi-axes equal to its scales).
DEFAULT_SCALE = 0.05

# Factor converting the first SH coefficient to a color: rgb = 0.5 + SH_C0 * coeff.
SH_C0 = 0.28209479177387814


def shDcForColor(rgb):
    """The SH0 coefficient that renders as the given 0 to 1 color."""
    return tuple((c - 0.5) / SH_C0 for c in rgb)


def cameraPosition(degrees=0.0, distance=8.0):
    """The world position of the persp camera placed by frameScene or orbitCamera."""
    angle = math.radians(degrees)
    return (distance * math.sin(angle), 0.0, distance * math.cos(angle))


def localViewDirection(splatPosition, camera, worldMatrix=None):
    """
    The unit direction from the camera to a splat, in the local space of the prim
    whose world matrix (Gf.Matrix4d) is worldMatrix (identity by default).
    """
    matrix = worldMatrix or Gf.Matrix4d(1.0)
    worldPosition = matrix.Transform(Gf.Vec3d(*splatPosition))
    worldDirection = worldPosition - Gf.Vec3d(*camera)
    local = matrix.GetInverse().TransformDir(worldDirection)
    local.Normalize()
    return (local[0], local[1], local[2])


_SH_BASIS_1 = 0.4886025119029199
_SH_BASIS_2 = (1.0925484305920792, -1.0925484305920792, 0.31539156525252005,
               -1.0925484305920792, 0.5462742152960396)
_SH_BASIS_3 = (-0.5900435899266435, 2.890611442640554, -0.4570457994644658,
               0.3731763325901154, -0.4570457994644658, 1.445305721320277,
               -0.5900435899266435)


def shColorRaw(coeffs, direction, degree):
    """
    CPU evaluation of the SH color (the test oracle, never used by the plugin):
    the unclamped rgb for a camera-to-splat direction in the prim's local space,
    with the 3DGS convention rgb = 0.5 + sum(basis * coefficient).
    """
    x, y, z = direction
    basis = [SH_C0]
    if degree >= 1:
        basis += [-_SH_BASIS_1 * y, _SH_BASIS_1 * z, -_SH_BASIS_1 * x]
    if degree >= 2:
        xx, yy, zz, xy, yz, xz = x * x, y * y, z * z, x * y, y * z, x * z
        basis += [_SH_BASIS_2[0] * xy, _SH_BASIS_2[1] * yz,
                  _SH_BASIS_2[2] * (2.0 * zz - xx - yy),
                  _SH_BASIS_2[3] * xz, _SH_BASIS_2[4] * (xx - yy)]
    if degree >= 3:
        basis += [_SH_BASIS_3[0] * y * (3.0 * xx - yy),
                  _SH_BASIS_3[1] * xy * z,
                  _SH_BASIS_3[2] * y * (4.0 * zz - xx - yy),
                  _SH_BASIS_3[3] * z * (2.0 * zz - 3.0 * xx - 3.0 * yy),
                  _SH_BASIS_3[4] * x * (4.0 * zz - xx - yy),
                  _SH_BASIS_3[5] * z * (xx - yy),
                  _SH_BASIS_3[6] * x * (xx - 3.0 * yy)]
    return tuple(0.5 + sum(b * coeffs[i][c] for i, b in enumerate(basis)) for c in range(3))


def shColor(coeffs, direction, degree):
    """The SH color clamped to 0..1, as the shader draws it."""
    return tuple(min(max(c, 0.0), 1.0) for c in shColorRaw(coeffs, direction, degree))


def sampleCoefficients(count, seed, amplitude=0.6):
    """
    Deterministic SH coefficients (count rgb triples), each channel within
    +-amplitude, with a band-0 term that keeps the color mid-range.
    """
    result = []
    for i in range(count):
        scale = amplitude if i > 0 else 0.5
        result.append(tuple(scale * math.sin(1.7 * seed + 2.3 * i + 0.9 * c) for c in range(3)))
    return result


def quatAboutAxis(axis, degrees):
    """A (w, x, y, z) quaternion rotating by degrees about a unit axis."""
    half = math.radians(degrees) / 2.0
    s = math.sin(half)
    return (math.cos(half), axis[0] * s, axis[1] * s, axis[2] * s)


def splatSchemaAvailable():
    """True when the USD build has the UsdVol Gaussian splat schema."""
    return UsdVol is not None and hasattr(UsdVol, SPLAT_SCHEMA_NAME)


# ---------------------------------------------------------------------------
# Scene building
# ---------------------------------------------------------------------------

def newScene():
    """Start an empty Maya scene with the USD plugin loaded and the camera set."""
    cmds.file(force=True, new=True)
    # A new scene restores color management, which would change the colors read from the view.
    cmds.colorManagementPrefs(edit=True, cmEnabled=False)
    mayaUtils.loadPlugin('mayaUsdPlugin')
    ufe.GlobalSelection.get().clear()


def createStage():
    """Create an in-memory stage under a proxy shape. Returns (shapeNode, stage)."""
    shapeNode, stage = mayaUtils.createProxyAndStage()
    UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)
    return shapeNode, stage


def defineSplat(stage, path, positions, scales=None, opacities=None,
                orientations=None, shCoeffs=None, shDegree=None, xform=None):
    """
    Define a splat prim from per-splat lists. Missing arrays are not authored,
    so that tests can exercise the renderer defaults.
    """
    prim = stage.DefinePrim(path, SPLAT_SCHEMA_NAME)
    positionsAttr = prim.CreateAttribute(ATTR_POSITIONS, Sdf.ValueTypeNames.Point3fArray)
    positionsAttr.Set(Vt.Vec3fArray([Gf.Vec3f(*p) for p in positions]))
    if scales is not None:
        prim.CreateAttribute(ATTR_SCALES, Sdf.ValueTypeNames.Float3Array).Set(
            Vt.Vec3fArray([Gf.Vec3f(*s) for s in scales]))
    if opacities is not None:
        prim.CreateAttribute(ATTR_OPACITIES, Sdf.ValueTypeNames.FloatArray).Set(
            Vt.FloatArray(list(opacities)))
    if orientations is not None:
        prim.CreateAttribute(ATTR_ORIENTATIONS, Sdf.ValueTypeNames.QuatfArray).Set(
            Vt.QuatfArray([Gf.Quatf(*q) for q in orientations]))
    if shCoeffs is not None:
        prim.CreateAttribute(ATTR_SH_COEFFS, Sdf.ValueTypeNames.Float3Array).Set(
            Vt.Vec3fArray([Gf.Vec3f(*c) for c in shCoeffs]))
    if shDegree is not None:
        prim.CreateAttribute(ATTR_SH_DEGREE, Sdf.ValueTypeNames.Int).Set(int(shDegree))
    if xform is not None:
        xformable = UsdGeom.Xformable(prim)
        xformable.AddTransformOp().Set(xform)
    return prim


def readPly(path):
    """
    Read an INRIA-style binary little-endian 3D Gaussian Splat PLY and return its
    splats as numpy arrays in the layout of the splat prim: positions (n, 3),
    scales (n, 3, activated with exp), orientations (n, 4, w first, normalized),
    opacities (n, activated with sigmoid), shCoeffs (n, coefficientCount, 3) and
    shDegree.
    """
    with open(path, 'rb') as plyFile:
        names, count, offset = [], 0, 0
        while True:
            line = plyFile.readline().decode('ascii', 'replace').strip()
            offset = plyFile.tell()
            if line.startswith('element vertex'):
                count = int(line.split()[2])
            elif line.startswith('property float'):
                names.append(line.split()[2])
            elif line.startswith('format') and 'binary_little_endian' not in line:
                raise RuntimeError('Unsupported PLY format in %s: %s' % (path, line))
            elif line == 'end_header':
                break
            elif line.startswith('property') or not line:
                raise RuntimeError('Unsupported PLY header in %s: %s' % (path, line))
    data = np.fromfile(path, dtype='<f4', count=count * len(names), offset=offset)
    data = data.reshape(count, len(names))
    column = {name: i for i, name in enumerate(names)}

    def columns(*propertyNames):
        return data[:, [column[n] for n in propertyNames]]

    restCount = sum(1 for n in names if n.startswith('f_rest_'))
    perChannel = restCount // 3
    shDegree = int(round(math.sqrt(perChannel + 1))) - 1
    coeffs = np.empty((count, perChannel + 1, 3), dtype=np.float32)
    coeffs[:, 0, :] = columns('f_dc_0', 'f_dc_1', 'f_dc_2')
    if perChannel:
        # f_rest is channel-major: all red coefficients, then green, then blue.
        rest = columns(*['f_rest_%d' % i for i in range(restCount)])
        coeffs[:, 1:, :] = rest.reshape(count, 3, perChannel).transpose(0, 2, 1)
    rotations = columns('rot_0', 'rot_1', 'rot_2', 'rot_3')
    norms = np.maximum(np.linalg.norm(rotations, axis=1, keepdims=True), 1e-12)
    return {
        'positions': np.ascontiguousarray(columns('x', 'y', 'z')),
        'scales': np.exp(columns('scale_0', 'scale_1', 'scale_2')),
        'orientations': np.ascontiguousarray(rotations / norms),
        'opacities': (1.0 / (1.0 + np.exp(-columns('opacity')[:, 0]))).astype(np.float32),
        'shCoeffs': coeffs,
        'shDegree': shDegree,
    }


def definePlySplat(stage, path, plyPath, xform=None):
    """
    Define a splat prim from an INRIA PLY file. Returns (prim, splats), where
    splats is the readPly result.
    """
    splats = readPly(plyPath)
    count = splats['positions'].shape[0]
    prim = stage.DefinePrim(path, SPLAT_SCHEMA_NAME)

    def author(name, typeName, value):
        prim.CreateAttribute(name, typeName).Set(value)

    author(ATTR_POSITIONS, Sdf.ValueTypeNames.Point3fArray,
           Vt.Vec3fArray.FromNumpy(np.ascontiguousarray(splats['positions'], dtype=np.float32)))
    author(ATTR_SCALES, Sdf.ValueTypeNames.Float3Array,
           Vt.Vec3fArray.FromNumpy(np.ascontiguousarray(splats['scales'], dtype=np.float32)))
    author(ATTR_OPACITIES, Sdf.ValueTypeNames.FloatArray,
           Vt.FloatArray.FromNumpy(splats['opacities']))
    orientations = np.ascontiguousarray(splats['orientations'], dtype=np.float32)
    try:
        quats = Vt.QuatfArray.FromNumpy(orientations)
    except (AttributeError, TypeError, Exception):
        quats = Vt.QuatfArray([Gf.Quatf(float(q[0]), float(q[1]), float(q[2]), float(q[3]))
                               for q in orientations])
    author(ATTR_ORIENTATIONS, Sdf.ValueTypeNames.QuatfArray, quats)
    author(ATTR_SH_COEFFS, Sdf.ValueTypeNames.Float3Array,
           Vt.Vec3fArray.FromNumpy(np.ascontiguousarray(
               splats['shCoeffs'].reshape(-1, 3), dtype=np.float32)))
    author(ATTR_SH_DEGREE, Sdf.ValueTypeNames.Int, int(splats['shDegree']))
    if xform is not None:
        UsdGeom.Xformable(prim).AddTransformOp().Set(xform)
    return prim, splats


def setSplatArrays(prim, positions, scales, opacities, orientations, shCoeffs):
    """Replace every per-splat array of a splat prim (the splat count may change)."""
    prim.GetAttribute(ATTR_POSITIONS).Set(Vt.Vec3fArray([Gf.Vec3f(*p) for p in positions]))
    prim.GetAttribute(ATTR_SCALES).Set(Vt.Vec3fArray([Gf.Vec3f(*v) for v in scales]))
    prim.GetAttribute(ATTR_OPACITIES).Set(Vt.FloatArray(list(opacities)))
    prim.GetAttribute(ATTR_ORIENTATIONS).Set(Vt.QuatfArray([Gf.Quatf(*q) for q in orientations]))
    prim.GetAttribute(ATTR_SH_COEFFS).Set(Vt.Vec3fArray([Gf.Vec3f(*c) for c in shCoeffs]))


def defaultPointsColor():
    """The 0..1 color Maya uses for dormant particles (the default color of splats)."""
    index = cmds.displayColor('particle', query=True, dormant=True)
    return tuple(cmds.colorIndex(index, query=True))


def gridPositions(count, spacing):
    """A count x count grid of positions in the XY plane, centered on the origin."""
    half = (count - 1) * spacing / 2.0
    return [(i * spacing - half, j * spacing - half, 0.0)
            for i in range(count) for j in range(count)]


def defineGridSplat(stage, path, count=10, spacing=0.2, scale=0.08,
                    opacity=1.0, shDc=(1.0, 0.0, 0.0), xform=None):
    """A small, fully covered grid splat prim with one SH0 color for all splats."""
    positions = gridPositions(count, spacing)
    n = len(positions)
    return defineSplat(stage, path, positions,
                       scales=[(scale, scale, scale)] * n,
                       opacities=[opacity] * n,
                       orientations=[(1.0, 0.0, 0.0, 0.0)] * n,
                       shCoeffs=[shDc] * n, shDegree=0, xform=xform)


def frameScene(distance=8.0):
    """Place the persp camera on the +Z axis looking at the origin (Y up)."""
    cmds.setAttr('persp.translate', 0, 0, distance, type='float3')
    cmds.setAttr('persp.rotate', 0, 0, 0, type='float3')


def orbitCamera(degrees, distance=8.0):
    """Place the persp camera on a circle around the Y axis, looking at the origin."""
    cmds.setAttr('persp.translate', *cameraPosition(degrees, distance), type='float3')
    cmds.setAttr('persp.rotate', 0, degrees, 0, type='float3')


def orbitAround(center, radius, degrees, distanceFactor=2.4):
    """
    Place the persp camera on a circle around the Y axis through center, far enough
    to see a sphere of the given radius, looking at center (Y up).
    """
    distance = radius * distanceFactor
    angle = math.radians(degrees)
    cmds.setAttr('persp.translate', center[0] + distance * math.sin(angle), center[1],
                 center[2] + distance * math.cos(angle), type='float3')
    cmds.setAttr('persp.rotate', 0, degrees, 0, type='float3')
    cmds.setAttr('perspShape.farClipPlane', max(1000.0, distance * 10.0))
    cmds.setAttr('perspShape.nearClipPlane', max(0.1, distance * 0.001))


def boundingSphere(positions):
    """(center, radius) of the bulk of the splat positions, ignoring far outliers."""
    center = np.median(positions, axis=0)
    distances = np.linalg.norm(positions - center, axis=1)
    return tuple(float(c) for c in center), float(np.percentile(distances, 98))


def centerRegion(halfSize, image=None):
    """A (x0, y0, x1, y1) square around the center of the snapshot (or of image)."""
    width = image.width() if image else SNAPSHOT_WIDTH
    height = image.height() if image else SNAPSHOT_HEIGHT
    cx, cy = width // 2, height // 2
    return (cx - halfSize, cy - halfSize, cx + halfSize, cy + halfSize)


def viewImage():
    """
    Refresh the active 3d view and read its color buffer as a QImage. Unlike a
    playblast snapshot it can be repeated in one scene, at the size of the view.
    """
    cmds.displayPref(displayGradient=False)
    cmds.displayRGBColor('background', 0.36, 0.36, 0.36)
    cmds.modelEditor(activePanel(), edit=True, grid=False, displayLights='default')
    cmds.refresh(force=True)
    view = omui.M3dView.active3dView()
    image = om.MImage()
    view.readColorBuffer(image, True)
    width, height = image.getSize()
    buffer = (ctypes.c_ubyte * (width * height * 4)).from_address(image.pixels())
    pixels = np.frombuffer(buffer, dtype=np.uint8).reshape(height, width, 4).copy()
    return QtGui.QImage(pixels.data, width, height, width * 4, QtGui.QImage.Format_RGBA8888).copy()


def activePanel():
    return mayaUtils.activeModelPanel()


def setViewportMode(textured=False, defaultMaterial=False):
    """Smooth shaded viewport, textured or not, with or without the default material."""
    cmds.modelEditor(activePanel(), edit=True, displayAppearance='smoothShaded',
                     displayTextures=textured, useDefaultMaterial=defaultMaterial,
                     wireframeOnShaded=False)


def setWireframeViewport():
    """Wireframe viewport."""
    cmds.modelEditor(activePanel(), edit=True, displayAppearance='wireframe',
                     wireframeOnShaded=False)


def setWireframeOnShaded(on=True):
    """Smooth shaded viewport with wireframe on shaded on or off."""
    cmds.modelEditor(activePanel(), edit=True, displayAppearance='smoothShaded',
                     wireframeOnShaded=on)


def refreshFrames(count=1):
    for _ in range(count):
        cmds.refresh(force=True)


def _profile(work):
    """Run work() under the Maya profiler, then one forced refresh. Returns the event count."""
    cmds.profiler(sampling=False)
    cmds.profiler(reset=True)
    cmds.profiler(bufferSize=200000)
    cmds.profiler(sampling=True)
    try:
        work()
        cmds.refresh(force=True)
    finally:
        cmds.profiler(sampling=False)
    return cmds.profiler(query=True, eventCount=True)


def profileEvents(work, scopeNames):
    """
    Run work() under the Maya profiler, then one forced refresh. Returns the
    number of recorded events per scope description of scopeNames.
    """
    counts = dict.fromkeys(scopeNames, 0)
    for i in range(_profile(work)):
        description = cmds.profiler(query=True, eventIndex=i, eventDescription=True)
        if description in counts:
            counts[description] += 1
    return counts


def profileEventNames(work, scopeName):
    """
    Run work() under the Maya profiler, then one forced refresh. Returns the set of
    event names of the scope described by scopeName (for HdVP2GaussianSplat::Sync,
    the ids of the synced Rprims).
    """
    names = set()
    for i in range(_profile(work)):
        if cmds.profiler(query=True, eventIndex=i, eventDescription=True) == scopeName:
            names.add(cmds.profiler(query=True, eventIndex=i, eventName=True))
    return names


# ---------------------------------------------------------------------------
# Instancing: baked twin scene
# ---------------------------------------------------------------------------

def defineInstancedSplat(stage, path, prototypeBuilder, instanceXforms):
    """
    Define a PointInstancer at path with one prototype splat prim built by
    prototypeBuilder(stage, prototypePath), and one instance per transform
    (Gf.Matrix4d) in instanceXforms.
    """
    instancer = UsdGeom.PointInstancer.Define(stage, path)
    protoPath = path + '/Prototypes/splat'
    stage.DefinePrim(path + '/Prototypes', 'Scope')
    prototypeBuilder(stage, protoPath)
    instancer.CreatePrototypesRel().SetTargets([Sdf.Path(protoPath)])
    _setInstanceArrays(instancer.CreatePositionsAttr(), instancer.CreateOrientationsAttr(),
                       instancer.CreateScalesAttr(), instancer.CreateProtoIndicesAttr(),
                       instanceXforms)
    return instancer


def _setInstanceArrays(positionsAttr, orientationsAttr, scalesAttr, protoIndicesAttr,
                       instanceXforms):
    """Author the point instancer arrays of one prototype from instance transforms."""
    positions, orients, scales = [], [], []
    for m in instanceXforms:
        t = Gf.Transform(m)
        positions.append(Gf.Vec3f(t.GetTranslation()))
        orients.append(Gf.Quath(t.GetRotation().GetQuat()))
        scales.append(Gf.Vec3f(t.GetScale()))
    positionsAttr.Set(Vt.Vec3fArray(positions))
    orientationsAttr.Set(Vt.QuathArray(orients))
    scalesAttr.Set(Vt.Vec3fArray(scales))
    protoIndicesAttr.Set(Vt.IntArray([0] * len(positions)))


def defineBakedTwin(stage, rootPath, prototypeBuilder, instanceXforms):
    """
    Define the twin of defineInstancedSplat: one plain splat prim per instance,
    with the instance transform applied on the prim. The twin renders the same
    picture as the instanced scene.
    """
    stage.DefinePrim(rootPath, 'Scope')
    prims = []
    for i, m in enumerate(instanceXforms):
        primPath = '%s/instance%d' % (rootPath, i)
        prims.append(prototypeBuilder(stage, primPath))
        UsdGeom.Xformable(prims[-1]).AddTransformOp().Set(m)
    return prims


def instanceXform(translate=(0.0, 0.0, 0.0), rotateDegrees=0.0, axis=(0, 0, 1), scale=1.0):
    """A Gf.Matrix4d that scales, then rotates, then translates (the point instancer order)."""
    scaleMatrix = Gf.Matrix4d(1.0)
    scaleMatrix.SetScale(scale)
    rotateMatrix = Gf.Matrix4d(1.0)
    rotateMatrix.SetRotate(Gf.Rotation(Gf.Vec3d(*axis), rotateDegrees))
    translateMatrix = Gf.Matrix4d(1.0)
    translateMatrix.SetTranslate(Gf.Vec3d(*translate))
    return scaleMatrix * rotateMatrix * translateMatrix


def setInstanceXforms(instancer, instanceXforms):
    """Replace the instances of a point instancer (the count may change, even to zero)."""
    _setInstanceArrays(instancer.GetPositionsAttr(), instancer.GetOrientationsAttr(),
                       instancer.GetScalesAttr(), instancer.GetProtoIndicesAttr(),
                       instanceXforms)


def defineNativeInstances(stage, prototypeBuilder, instanceXforms, root='/Scene'):
    """
    Define native instances: one instanceable prim root/InstanceN per transform,
    all referencing a class prim whose splat prim is built by
    prototypeBuilder(stage, path). Returns the instance prims.
    """
    source = '/__SplatSource'
    stage.CreateClassPrim(source)
    prototypeBuilder(stage, source + '/splat')
    stage.DefinePrim(root, 'Xform')
    prims = []
    for i, m in enumerate(instanceXforms):
        prim = stage.DefinePrim('%s/Instance%d' % (root, i), 'Xform')
        prim.GetReferences().AddInternalReference(source)
        prim.SetInstanceable(True)
        UsdGeom.Xformable(prim).AddTransformOp().Set(m)
        prims.append(prim)
    return prims


def viewInstancedAndTwin(instancedBuilder, twinBuilder, viewSetup=None):
    """
    Render two scenes, one built by instancedBuilder(stage) and one by twinBuilder(stage),
    each in a new scene with a fresh stage, and return the two view color buffer images.
    viewSetup() runs after the scene is built (a new scene resets the viewport).
    """
    images = []
    for builder in (instancedBuilder, twinBuilder):
        newScene()
        shapeNode, stage = createStage()
        builder(stage)
        frameScene()
        if viewSetup:
            viewSetup()
        images.append(viewImage())
    return images


def differingFraction(imageA, imageB, tolerance=8):
    """Fraction of the pixels of two images of the same size that differ by more than tolerance."""
    diff = np.abs(imageArray(imageA).astype(np.int16) - imageArray(imageB).astype(np.int16))
    return float((diff.max(axis=2) > tolerance).sum()) / float(imageA.width() * imageA.height())


def middleBand(image):
    """A (x0, y0, x1, y1) band across the middle of the image, clear of the view cube."""
    return (0, image.height() // 4, image.width(), image.height() * 3 // 4)


def _bandColors(image, parts):
    """
    The dominant colors of parts equal columns of the middle band of the image, left to
    right, None for a column where less than 1% of the pixels are drawn.
    """
    width = image.width()
    top, bottom = image.height() // 4, image.height() * 3 // 4
    colors = []
    for i in range(parts):
        region = (width * i // parts, top, width * (i + 1) // parts, bottom)
        drawn = coverage(image, region=region) >= 0.01
        colors.append(dominantColor(image, region=region) if drawn else None)
    return tuple(colors)


def thirdColors(image):
    """
    The dominant colors of the left, middle and right third of the middle band, None for a
    third where less than 1% of the pixels are drawn.
    """
    return _bandColors(image, 3)


def widgetPosition(worldPoint):
    """The position (QPoint) in the 3d view widget of a world point, for mouse events."""
    view = omui.M3dView.active3dView()
    x, y = view.worldToView(om.MPoint(*worldPoint))[:2]
    widget = viewWidget()
    return QtCore.QPoint(int(round(x * widget.width() / float(view.portWidth()))),
                         int(round((view.portHeight() - y) * widget.height() / float(view.portHeight()))))


# ---------------------------------------------------------------------------
# Snapshot measurements
# ---------------------------------------------------------------------------

def snapshotImage(imageName, directory=None):
    """Take a 960x540 snapshot and return it as a QImage."""
    directory = directory or os.path.abspath('.')
    path = os.path.join(directory, imageName)
    imageUtils.snapshot(path, width=SNAPSHOT_WIDTH, height=SNAPSHOT_HEIGHT)
    image = QtGui.QImage(path)
    if image.isNull():
        raise RuntimeError('Could not read the snapshot %s' % path)
    return image


def _pixel(image, x, y):
    c = QtGui.QColor(image.pixel(x, y))
    return (c.red(), c.green(), c.blue())


def imageArray(image):
    """The image as a numpy array of shape (height, width, 3), RGB, uint8."""
    converted = image.convertToFormat(QtGui.QImage.Format_RGB32)
    width, height = converted.width(), converted.height()
    raw = np.frombuffer(converted.constBits(), dtype=np.uint8)
    # Format_RGB32 is stored as B, G, R, 255 per pixel, rows padded to bytesPerLine.
    rows = raw[:converted.bytesPerLine() * height].reshape(height, converted.bytesPerLine())
    pixels = rows[:, :width * 4].reshape(height, width, 4)
    return np.ascontiguousarray(pixels[:, :, [2, 1, 0]])


def backgroundColor(image):
    """The background color, read from the top-left corner of the image."""
    return _pixel(image, 0, 0)


def foregroundArray(image, tolerance=6):
    """Boolean array (height, width) of the pixels that differ from the background color."""
    pixels = imageArray(image).astype(np.int16)
    bg = np.array(backgroundColor(image), dtype=np.int16)
    return np.abs(pixels - bg).max(axis=2) > tolerance


def foregroundMask(image, tolerance=6, region=None):
    """
    A set of (x, y) pixels that differ from the background color. region is an
    optional (x0, y0, x1, y1) rectangle the set is restricted to.
    """
    x0, y0, x1, y1 = region or (0, 0, image.width(), image.height())
    sub = foregroundArray(image, tolerance)[y0:y1, x0:x1]
    return {(int(x) + x0, int(y) + y0) for (y, x) in np.argwhere(sub)}


def coverage(image, tolerance=6, region=None):
    """
    Fraction of pixels that differ from the background. region is an optional
    (x0, y0, x1, y1) rectangle, and the fraction is taken over that rectangle.
    """
    x0, y0, x1, y1 = region or (0, 0, image.width(), image.height())
    sub = foregroundArray(image, tolerance)[y0:y1, x0:x1]
    return float(sub.sum()) / float(sub.size)


def dominantColor(image, tolerance=6, region=None):
    """Average color of the foreground pixels, or None when nothing is drawn."""
    x0, y0, x1, y1 = region or (0, 0, image.width(), image.height())
    mask = foregroundArray(image, tolerance)[y0:y1, x0:x1]
    if not mask.any():
        return None
    pixels = imageArray(image)[y0:y1, x0:x1]
    return tuple(float(v) for v in pixels[mask].mean(axis=0))


def distinctColors(image, region, quantize=8):
    """The set of distinct colors (quantized) of the foreground pixels of a region."""
    x0, y0, x1, y1 = region
    mask = foregroundArray(image)[y0:y1, x0:x1]
    pixels = (imageArray(image)[y0:y1, x0:x1][mask] // quantize)
    return {tuple(int(c) for c in p) for p in np.unique(pixels, axis=0)}


def halfColors(image):
    """
    The dominant colors of the left and of the right half of the middle band of the image,
    or None for a half where less than 1% of the pixels are drawn.
    """
    return _bandColors(image, 2)


def centerDominantColor(image, halfSize=20):
    """The dominant color of a small square around the center of the image."""
    return dominantColor(image, region=centerRegion(halfSize, image))


def ellipseAxes(mask):
    """
    Measure the ellipse of a pixel cluster from its second moments. Returns
    (majorLength, minorLength, angleRadians), the lengths being full axis
    lengths in pixels of the uniform ellipse with the same moments, and the
    angle the direction of the major axis from +x (y pointing down).
    """
    if not mask:
        raise ValueError('Cannot measure the ellipse of an empty cluster')
    n = float(len(mask))
    mx = sum(p[0] for p in mask) / n
    my = sum(p[1] for p in mask) / n
    sxx = sum((p[0] - mx) ** 2 for p in mask) / n
    syy = sum((p[1] - my) ** 2 for p in mask) / n
    sxy = sum((p[0] - mx) * (p[1] - my) for p in mask) / n
    trace_half = (sxx + syy) / 2.0
    root = math.sqrt(max(((sxx - syy) / 2.0) ** 2 + sxy ** 2, 0.0))
    l1, l2 = trace_half + root, max(trace_half - root, 0.0)
    angle = 0.5 * math.atan2(2.0 * sxy, sxx - syy)
    # A uniform filled ellipse with semi-axis a has variance a^2 / 4.
    return (4.0 * math.sqrt(l1), 4.0 * math.sqrt(l2), angle)


def boundingSize(mask):
    """(width, height) in pixels of the bounding box of a pixel set."""
    if not mask:
        return (0, 0)
    xs = [p[0] for p in mask]
    ys = [p[1] for p in mask]
    return (max(xs) - min(xs) + 1, max(ys) - min(ys) + 1)


def clusters(mask):
    """The 8-connected clusters of a pixel set, as a list of sets."""
    remaining = set(mask)
    result = []
    while remaining:
        seed = remaining.pop()
        cluster, stack = {seed}, [seed]
        while stack:
            x, y = stack.pop()
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    q = (x + dx, y + dy)
                    if q in remaining:
                        remaining.remove(q)
                        cluster.add(q)
                        stack.append(q)
        result.append(cluster)
    return result


def largestCluster(mask):
    """The largest 8-connected cluster of a pixel set."""
    return max(clusters(mask), key=len, default=set())


def imagesEqual(imageA, imageB, tolerance=0, maxDifferingFraction=0.0):
    """
    True when the two images have the same size and at most
    maxDifferingFraction of their pixels differ by more than tolerance.
    """
    if imageA.size() != imageB.size():
        return False
    return differingFraction(imageA, imageB, tolerance) <= maxDifferingFraction


# ---------------------------------------------------------------------------
# Log capture
# ---------------------------------------------------------------------------

class LogCapture(object):
    """
    Context manager capturing Maya's command output (info, warnings and errors,
    which includes TF_WARN and MGlobal::displayWarning from the plugin).

    The maya-usd diagnostic delegate holds the USD diagnostics issued on worker threads
    (Hydra Sync) until the main thread is idle, and merges a burst of them by call site
    into one "-- and N similar" line. The capture raises the burst limit, so that every
    diagnostic is printed on its own, and flushes the delegate when it starts (dropping
    what was issued before) and before it stops (keeping what was issued during).
    """

    # Diagnostics in one second before the delegate merges them by call site.
    UNBATCHED_LIMIT = 1000000

    def __init__(self):
        self.messages = []
        self._callbackId = None
        self._previousLimit = None

    def _onMessage(self, message, messageType, clientData):
        self.messages.append((messageType, message))

    def start(self):
        delegate = mayaUsdLib.DiagnosticDelegate
        delegate.Flush()
        self._previousLimit = delegate.GetMaximumUnbatchedDiagnostics()
        delegate.SetMaximumUnbatchedDiagnostics(self.UNBATCHED_LIMIT)
        self._callbackId = om.MCommandMessage.addCommandOutputCallback(self._onMessage)

    def stop(self):
        if self._callbackId is None:
            return
        delegate = mayaUsdLib.DiagnosticDelegate
        try:
            delegate.Flush()
        finally:
            om.MMessage.removeCallback(self._callbackId)
            self._callbackId = None
            delegate.SetMaximumUnbatchedDiagnostics(self._previousLimit)

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args):
        self.stop()

    def clear(self):
        self.messages = []

    def _texts(self, types=None, containing=None):
        """Captured messages of the given types (all by default) containing a substring."""
        return [m for (t, m) in self.messages
                if (types is None or t in types) and (containing is None or containing in m)]

    def lines(self, containing=None):
        """Captured messages, optionally only those containing a substring."""
        return self._texts(containing=containing)

    def errors(self, containing=None):
        return self._texts((om.MCommandMessage.kError,), containing)

    def warnings(self, containing=None):
        """Warnings and errors."""
        return self._texts((om.MCommandMessage.kWarning, om.MCommandMessage.kError), containing)


def drawnItemCounts():
    """
    Refresh once with the VP2 render pipeline trace on, and return the number of render
    items drawn per render list of the beauty pass ({'Opaque': 1, ...}), summed over the
    views. Splat items are opaque, so each drawn splat item counts once in 'Opaque'.
    """
    counts = {}
    with LogCapture() as log:
        cmds.ogs(traceRenderPipeline=True)
        try:
            cmds.refresh(force=True)
        finally:
            cmds.ogs(traceRenderPipeline=False)
    for line in log.lines(containing='Count for list: ['):
        name, _, count = line.split('Count for list: [', 1)[1].partition('] = ')
        counts[name] = counts.get(name, 0) + int(count)
    return counts


# ---------------------------------------------------------------------------
# Selection and picking
# ---------------------------------------------------------------------------

def ufeItem(shapeNode, primPath):
    """The UFE scene item of a prim (a path like '/splat') under the proxy shape."""
    shapePath = cmds.ls(shapeNode, long=True)[0]
    ufePath = ufe.Path([mayaUtils.createUfePathSegment(shapePath),
                        usdUtils.createUfePathSegment(primPath)])
    return ufe.Hierarchy.createItem(ufePath)


def select(shapeNode, primPaths):
    """Select prims through ufe.GlobalSelection (a path, or a list of paths)."""
    if isinstance(primPaths, str):
        primPaths = [primPaths]
    selection = ufe.Selection()
    for p in primPaths:
        selection.append(ufeItem(shapeNode, p))
    ufe.GlobalSelection.get().replaceWith(selection)


def clearSelection():
    ufe.GlobalSelection.get().clear()


def selectedPathStrings():
    return [ufe.PathString.string(item.path()) for item in ufe.GlobalSelection.get()]


def viewWidget():
    view = omui.M3dView.active3dView()
    return wrapInstance(int(view.widget()), QWidget)


def clickInView(localPos=None):
    """Pick with a Qt mouse click at localPos (QPoint), the view center by default."""
    widget = viewWidget()
    qtInputHelpers.send_mouse_click(widget, localPos or widget.rect().center())
