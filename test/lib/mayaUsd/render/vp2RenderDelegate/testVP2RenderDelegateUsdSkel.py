#!/usr/bin/env mayapy
#
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

import fixturesUtils
import imageUtils
import mayaUtils
import usdUtils
import testUtils

from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from maya import cmds

from pxr import Gf, Usd, UsdGeom, UsdSkel, Vt

import ufe

import math
import os


class testVP2RenderDelegateUsdSkel(imageUtils.ImageDiffingTestCase):
    """
    Tests imaging using the Viewport 2.0 render delegate when using per-instance
    inherited data on instances.
    """

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        inputPath = fixturesUtils.setUpClass(__file__,
            initializeStandalone=False, loadPlugin=False)

        cls._baselineDir = os.path.join(inputPath,
            'VP2RenderDelegateUsdSkelTest', 'baseline')

        cls._testDir = os.path.abspath('.')

    def assertSnapshotClose(self, imageName):
        baselineImage = os.path.join(self._baselineDir, imageName)
        snapshotImage = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(snapshotImage, width=960, height=540)
        return self.assertImagesClose(baselineImage, snapshotImage)

    def _StartTest(self, testName):
        self._testName = testName
        testFile = testUtils.getTestScene("UsdSkel", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()
        cmds.currentTime(0)
        self.assertSnapshotClose('%s_0.png' % self._testName)
        cmds.currentTime(50)
        self.assertSnapshotClose('%s_50.png' % self._testName)
        cmds.currentTime(100)
        self.assertSnapshotClose('%s_100.png' % self._testName)

    def testPerInstanceInheritedData(self):
        mayaUtils.loadPlugin("mayaUsdPlugin")

        cmds.file(force=True, new=True)
        cmds.move(-8, 15, 6, 'persp')
        cmds.rotate(70, 0, -160, 'persp')
        self._StartTest('skinCluster')

        cmds.file(force=True, new=True)
        cmds.move(180, -525, 225, 'persp')
        cmds.rotate(75, 0, 0, 'persp')
        self._StartTest('HIK_Export')


    NUM_STRANDS = 8
    ROWS = 8
    HEIGHT = 2.0
    RADIUS = 0.45
    END_FRAME = 30
    COMPARE_FRAME = 30
    BEND_DEGREES = 100.0

    # The strands cover a small share of the frame, so the average per-channel
    # difference stays modest even when they swing a long way. Measured diffs are
    # ~0.0033 for curves (VP2 draws those as thin lines) and ~0.0098 for points,
    # i.e. 3x and 10x this threshold. A prim frozen at its rest pose gives 0.
    MIN_DEFORM_DIFF = 1e-03

    def _NewStage(self, fileName):
        stage = Usd.Stage.CreateNew(cmds.internalVar(utd=1) + '/' + fileName)
        UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.z)
        UsdGeom.SetStageMetersPerUnit(stage, 0.01)
        stage.SetStartTimeCode(0)
        stage.SetEndTimeCode(self.END_FRAME)
        stage.SetTimeCodesPerSecond(24)
        stage.SetFramesPerSecond(24)
        return stage

    def _DefineGprim(self, stage, primPath, kind, points):
        """Define the BasisCurves or Points prim that the test will look at."""
        if kind == 'Curves':
            gprim = UsdGeom.BasisCurves.Define(stage, primPath)
            gprim.CreateTypeAttr(UsdGeom.Tokens.linear)
            gprim.CreateWrapAttr(UsdGeom.Tokens.nonperiodic)
            gprim.CreateCurveVertexCountsAttr(
                Vt.IntArray([self.ROWS] * self.NUM_STRANDS))
            gprim.CreateWidthsAttr(Vt.FloatArray([0.05] * len(points)))
            gprim.SetWidthsInterpolation(UsdGeom.Tokens.vertex)
            gprim.CreateDisplayColorAttr(Vt.Vec3fArray([Gf.Vec3f(0.9, 0.35, 0.1)]))
        else:
            gprim = UsdGeom.Points.Define(stage, primPath)
            gprim.CreateWidthsAttr(Vt.FloatArray([0.16] * len(points)))
            gprim.CreateDisplayColorAttr(Vt.Vec3fArray([Gf.Vec3f(0.2, 0.6, 0.95)]))
        gprim.CreatePointsAttr(Vt.Vec3fArray(points))
        # Generous extent: it must still contain the geometry once deformed.
        gprim.CreateExtentAttr(Vt.Vec3fArray([
            Gf.Vec3f(-self.HEIGHT, -self.HEIGHT, -self.HEIGHT),
            Gf.Vec3f(self.HEIGHT, self.HEIGHT, self.HEIGHT * 1.5)]))
        return gprim

    def _WriteSkinnedScene(self, kind):
        """Vertical strands bound to a two joint skeleton that bends about X."""
        stage = self._NewStage('skinned%s.usda' % kind)
        root = '/SkelRoot' + kind
        stage.SetDefaultPrim(UsdSkel.Root.Define(stage, root).GetPrim())

        skel = UsdSkel.Skeleton.Define(stage, root + '/Skel')
        skel.CreateJointsAttr(Vt.TokenArray(['Root', 'Root/Bone1']))
        xforms = Vt.Matrix4dArray([
            Gf.Matrix4d(1.0),
            Gf.Matrix4d(1.0).SetTranslate(Gf.Vec3d(0, 0, self.HEIGHT * 0.5))])
        skel.CreateBindTransformsAttr(xforms)
        skel.CreateRestTransformsAttr(xforms)

        anim = UsdSkel.Animation.Define(stage, root + '/Skel/Anim')
        anim.CreateJointsAttr(Vt.TokenArray(['Root', 'Root/Bone1']))
        anim.CreateTranslationsAttr()
        anim.CreateRotationsAttr()
        anim.CreateScalesAttr()
        for frame in range(self.END_FRAME + 1):
            angle = self.BEND_DEGREES * (frame / float(self.END_FRAME))
            q = Gf.Rotation(Gf.Vec3d(1, 0, 0), angle).GetQuat()
            anim.GetTranslationsAttr().Set(Vt.Vec3fArray([
                Gf.Vec3f(0, 0, 0), Gf.Vec3f(0, 0, self.HEIGHT * 0.5)]), frame)
            anim.GetRotationsAttr().Set(Vt.QuatfArray([
                Gf.Quatf(1, 0, 0, 0),
                Gf.Quatf(float(q.GetReal()), Gf.Vec3f(*q.GetImaginary()))]), frame)
            anim.GetScalesAttr().Set(Vt.Vec3hArray([
                Gf.Vec3h(1, 1, 1), Gf.Vec3h(1, 1, 1)]), frame)
        UsdSkel.BindingAPI.Apply(skel.GetPrim()) \
            .CreateAnimationSourceRel().SetTargets([anim.GetPath()])

        points, heights = [], []
        for strand in range(self.NUM_STRANDS):
            a = 2.0 * math.pi * strand / self.NUM_STRANDS
            x, y = self.RADIUS * math.cos(a), self.RADIUS * math.sin(a)
            for row in range(self.ROWS):
                z = self.HEIGHT * row / float(self.ROWS - 1)
                points.append(Gf.Vec3f(x, y, z))
                heights.append(z)
        gprim = self._DefineGprim(stage, root + '/' + kind, kind, points)

        # Two influences per vertex, blending Root -> Bone1 with height, so the
        # strands bend rather than swing rigidly.
        indices, weights = [], []
        for z in heights:
            w = min(max(z / self.HEIGHT, 0.0), 1.0)
            indices.extend([0, 1])
            weights.extend([1.0 - w, w])
        binding = UsdSkel.BindingAPI.Apply(gprim.GetPrim())
        binding.CreateSkeletonRel().SetTargets([skel.GetPath()])
        binding.CreateGeomBindTransformAttr(Gf.Matrix4d(1.0))
        binding.CreateJointIndicesPrimvar(False, 2).Set(Vt.IntArray(indices))
        binding.CreateJointWeightsPrimvar(False, 2).Set(Vt.FloatArray(weights))

        stage.GetRootLayer().Save()
        return stage.GetRootLayer().identifier

    def _WriteBakedScene(self, kind, skinnedPath):
        """The skinned result at COMPARE_FRAME, baked into a plain static prim.

        Derived from the skinned scene, so it is by construction the pose VP2 is
        expected to draw. Also asserts the prim really is point-deformed - a
        rigidly deformed prim would move by transform and prove nothing here.
        """
        stage = Usd.Stage.Open(skinnedPath)
        root = '/SkelRoot' + kind
        cache = UsdSkel.Cache()
        cache.Populate(UsdSkel.Root(stage.GetPrimAtPath(root)),
                       Usd.TraverseInstanceProxies())

        prim = stage.GetPrimAtPath(root + '/' + kind)
        skinningQuery = cache.GetSkinningQuery(prim)
        self.assertIsNotNone(skinningQuery, '%s is not a skinning target' % kind)
        self.assertFalse(skinningQuery.IsRigidlyDeformed(),
                         '%s must be point-deformed, not rigid' % kind)
        skelQuery = cache.GetSkelQuery(
            UsdSkel.Skeleton(stage.GetPrimAtPath(root + '/Skel')))

        points = Vt.Vec3fArray(UsdGeom.PointBased(prim).GetPointsAttr().Get())
        self.assertTrue(skinningQuery.ComputeSkinnedPoints(
            skelQuery.ComputeSkinningTransforms(self.COMPARE_FRAME),
            points, self.COMPARE_FRAME))

        baked = self._NewStage('baked%s.usda' % kind)
        bakedRoot = '/Baked' + kind
        baked.SetDefaultPrim(UsdGeom.Xform.Define(baked, bakedRoot).GetPrim())
        self._DefineGprim(baked, bakedRoot + '/' + kind, kind, list(points))
        baked.GetRootLayer().Save()
        return baked.GetRootLayer().identifier

    def _SnapshotScene(self, usdFilePath, frame, imageName):
        """Render one scene from a fixed viewpoint.

        Deterministic camera placement rather than viewFit: the skinned and baked
        scenes must be rendered from exactly the same viewpoint to be comparable.
        """
        cmds.file(force=True, new=True)
        mayaUtils.createProxyFromFile(usdFilePath)
        ufe.GlobalSelection.get().clear()
        cmds.viewPlace('persp', eye=(7.5, 0, 1.0), la=(0, 0, 0.85), up=(0, 0, 1))
        cmds.currentTime(frame)
        imagePath = os.path.join(self._testDir, imageName)
        imageUtils.snapshot(imagePath, width=400, height=400)
        return imagePath

    def _AssertSkinnedPrimDeforms(self, kind):
        mayaUtils.loadPlugin("mayaUsdPlugin")
        skinnedPath = self._WriteSkinnedScene(kind)
        bakedPath = self._WriteBakedScene(kind, skinnedPath)

        # 1. The prim must actually move over the shot. Comparing every pair
        #    catches a prim that deforms once and then goes stale, as well as one
        #    that never deforms at all.
        frames = (0, self.COMPARE_FRAME // 2, self.COMPARE_FRAME)
        shots = [(f, self._SnapshotScene(
            skinnedPath, f, 'skinned%s_%d.png' % (kind, f))) for f in frames]

        for i in range(len(shots)):
            for j in range(i + 1, len(shots)):
                frameA, pathA = shots[i]
                frameB, pathB = shots[j]
                diff = imageUtils.imageDiff(pathA, pathB)
                self.assertGreater(
                    diff, self.MIN_DEFORM_DIFF,
                    'skinned%s did not deform between frame %d and frame %d (image '
                    'diff %g <= %g). The skinned points coming from the '
                    'HdExtComputation are most likely being ignored in favour of '
                    'the rest points from the scene delegate.'
                    % (kind, frameA, frameB, diff, self.MIN_DEFORM_DIFF))

        # 2. ...and it must land where UsdSkel says it should.
        posed = dict(shots)[self.COMPARE_FRAME]
        baked = self._SnapshotScene(
            bakedPath, self.COMPARE_FRAME, 'baked%s_ref.png' % kind)
        self.assertImagesClose(posed, baked)

    def testSkinnedBasisCurves(self):
        """UsdSkel must deform a BasisCurves prim, and deform it correctly."""
        self._AssertSkinnedPrimDeforms('Curves')

    def testSkinnedPoints(self):
        """UsdSkel must deform a Points prim, and deform it correctly."""
        self._AssertSkinnedPrimDeforms('Points')



if __name__ == '__main__':
    fixturesUtils.runTests(globals())
