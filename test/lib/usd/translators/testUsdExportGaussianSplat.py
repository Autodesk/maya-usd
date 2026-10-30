#!/usr/bin/env mayapy
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

import atexit
import os
import unittest

from maya import cmds

from pxr import Usd
from pxr import UsdVol

import fixturesUtils

# mtoa's Arnold runtime and mayaUsd's usdUfe module interact badly during normal process
# teardown on hosts with mtoa installed: a SIGSEGV happens in a static destructor well
# after every test has already reported its result. Track pass/fail ourselves and, once
# mtoa has been loaded, force-exit before that teardown runs instead of letting ctest see
# a false failure.
_testsFailed = False

def _recordFailure(origMethod):
    def wrapper(self, *args, **kwargs):
        global _testsFailed
        _testsFailed = True
        return origMethod(self, *args, **kwargs)
    return wrapper

unittest.TestResult.addFailure = _recordFailure(unittest.TestResult.addFailure)
unittest.TestResult.addError = _recordFailure(unittest.TestResult.addError)

def _forceExit():
    os._exit(1 if _testsFailed else 0)

class testUsdExportGaussianSplat(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        cls._plyDir = os.path.join(
            inputPath, os.pardir, os.pardir, os.pardir, "testSamples", "3dgs", "ply")

        cls._hasMtoaGaussianSplat = False
        try:
            cmds.loadPlugin('mtoa', quiet=True)
        except RuntimeError:
            pass
        else:
            if cmds.pluginInfo('mtoa', query=True, loaded=True):
                cls._hasMtoaGaussianSplat = cmds.attributeQuery(
                    'filename', type='aiGaussianSplat', exists=True)
                if cls._hasMtoaGaussianSplat:
                    atexit.register(_forceExit)

    def _assertSplatCounts(self, prim, count, degree):
        self.assertTrue(prim)
        self.assertEqual(len(prim.GetPositionsAttr().Get()), count)
        self.assertEqual(len(prim.GetScalesAttr().Get()), count)
        self.assertEqual(len(prim.GetOrientationsAttr().Get()), count)
        self.assertEqual(len(prim.GetOpacitiesAttr().Get()), count)
        self.assertEqual(prim.GetRadianceSphericalHarmonicsDegreeAttr().Get(), degree)
        self.assertEqual(
            len(prim.GetRadianceSphericalHarmonicsCoefficientsAttr().Get()),
            count * (degree + 1) ** 2)
        self.assertEqual(len(prim.GetExtentAttr().Get()), 2)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testExportDegree0(self):
        if not self._hasMtoaGaussianSplat:
            self.skipTest("mtoa with aiGaussianSplat is not available")

        cmds.file(new=True, force=True)

        node = cmds.createNode('aiGaussianSplat')
        plyPath = os.path.join(self._plyDir, "tennisball_degree0_ascii_10.ply")
        cmds.setAttr(node + '.filename', plyPath, type='string')
        transform = cmds.listRelatives(node, parent=True)[0]

        usdFile = os.path.abspath("UsdExportGaussianSplatTest_Degree0.usda")
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none')

        stage = Usd.Stage.Open(usdFile)
        prim = UsdVol.ParticleField3DGaussianSplat.Get(stage, '/' + transform)
        self._assertSplatCounts(prim, 10, 0)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testExportDegree1(self):
        if not self._hasMtoaGaussianSplat:
            self.skipTest("mtoa with aiGaussianSplat is not available")

        cmds.file(new=True, force=True)

        node = cmds.createNode('aiGaussianSplat')
        plyPath = os.path.join(self._plyDir, "tennisball_degree1_ascii_10.ply")
        cmds.setAttr(node + '.filename', plyPath, type='string')
        transform = cmds.listRelatives(node, parent=True)[0]

        usdFile = os.path.abspath("UsdExportGaussianSplatTest_Degree1.usda")
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none')

        stage = Usd.Stage.Open(usdFile)
        prim = UsdVol.ParticleField3DGaussianSplat.Get(stage, '/' + transform)
        self._assertSplatCounts(prim, 10, 1)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testExportDegree3IsAuthored(self):
        """
        Degree 3 is the schema fallback, but it must still be authored: Hydra ignores the
        SH coefficients (every splat renders grey) when the degree is not authored.
        """
        if not self._hasMtoaGaussianSplat:
            self.skipTest("mtoa with aiGaussianSplat is not available")

        cmds.file(new=True, force=True)

        node = cmds.createNode('aiGaussianSplat')
        plyPath = os.path.join(self._plyDir, "tennisball_degree3_binary_10.ply")
        cmds.setAttr(node + '.filename', plyPath, type='string')
        transform = cmds.listRelatives(node, parent=True)[0]

        usdFile = os.path.abspath("UsdExportGaussianSplatTest_Degree3.usda")
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, shadingMode='none')

        stage = Usd.Stage.Open(usdFile)
        prim = UsdVol.ParticleField3DGaussianSplat.Get(stage, '/' + transform)
        self._assertSplatCounts(prim, 10, 3)
        self.assertTrue(
            prim.GetRadianceSphericalHarmonicsDegreeAttr().HasAuthoredValue())

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testExportValues(self):
        """The first splat of the .ply fixture is exported with the right values."""
        if not self._hasMtoaGaussianSplat:
            self.skipTest("mtoa with aiGaussianSplat is not available")

        cmds.file(new=True, force=True)

        node = cmds.createNode('aiGaussianSplat')
        plyPath = os.path.join(self._plyDir, "tennisball_degree0_ascii_10.ply")
        cmds.setAttr(node + '.filename', plyPath, type='string')
        transform = cmds.listRelatives(node, parent=True)[0]

        usdFile = os.path.abspath("UsdExportGaussianSplatTest_Values.usda")
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, shadingMode='none')

        stage = Usd.Stage.Open(usdFile)
        prim = UsdVol.ParticleField3DGaussianSplat.Get(stage, '/' + transform)
        self.assertTrue(prim)

        # Maya's default linear unit (cm) matches the exported stage, so no unit scaling.
        position = prim.GetPositionsAttr().Get()[0]
        for actual, expected in zip(position, (-22.629562, 8.323967, -68.072632)):
            self.assertAlmostEqual(actual, expected, places=3)

        # The PLY stores the rotation as (w, x, y, z).
        orientation = prim.GetOrientationsAttr().Get()[0]
        self.assertAlmostEqual(orientation.GetReal(), -0.300490, places=4)
        for actual, expected in zip(orientation.GetImaginary(), (0.931352, 0.185303, 0.089176)):
            self.assertAlmostEqual(actual, expected, places=4)

        # The extent contains every splat position.
        extent = prim.GetExtentAttr().Get()
        for point in prim.GetPositionsAttr().Get():
            for axis in range(3):
                self.assertLessEqual(extent[0][axis], point[axis])
                self.assertGreaterEqual(extent[1][axis], point[axis])

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testExportMissingFile(self):
        """A missing .ply file must not crash the export and writes no splat data."""
        if not self._hasMtoaGaussianSplat:
            self.skipTest("mtoa with aiGaussianSplat is not available")

        cmds.file(new=True, force=True)

        node = cmds.createNode('aiGaussianSplat')
        cmds.setAttr(node + '.filename', os.path.join(self._plyDir, "doesNotExist.ply"),
            type='string')
        transform = cmds.listRelatives(node, parent=True)[0]

        usdFile = os.path.abspath("UsdExportGaussianSplatTest_MissingFile.usda")
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, shadingMode='none')

        stage = Usd.Stage.Open(usdFile)
        prim = UsdVol.ParticleField3DGaussianSplat.Get(stage, '/' + transform)
        if prim:
            self.assertFalse(prim.GetPositionsAttr().HasAuthoredValue())

if __name__ == '__main__':
    unittest.main(verbosity=2)
