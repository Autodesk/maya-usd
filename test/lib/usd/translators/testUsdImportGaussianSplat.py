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
import math
import os
import struct
import unittest

from maya import cmds

from pxr import Gf
from pxr import Usd
from pxr import UsdVol
from pxr import Vt

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

# Exactly representable in half precision, so float and half inputs give the same result.
_POSITIONS = [(0.0, 0.0, 0.0), (1.0, 2.0, 3.0), (-0.5, 0.25, -4.0)]

_SAMPLES_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'testSamples', '3dgs')

def _readPlyVertices(plyPath):
    """Returns (propertyNames, vertices) of a binary .ply file with only float properties."""
    with open(plyPath, 'rb') as f:
        data = f.read()
    headerEnd = data.index(b'end_header\n') + len(b'end_header\n')
    header = data[:headerEnd].decode('ascii').splitlines()
    plyFormat = [line for line in header if line.startswith('format')][0]
    assert plyFormat.startswith('format binary_little_endian'), plyFormat
    count = int([line for line in header if line.startswith('element vertex')][0].split()[2])
    names = [line.split()[2] for line in header if line.startswith('property float')]
    propertyCount = len([line for line in header if line.startswith('property ')])
    assert propertyCount == len(names), 'Only float properties are supported'
    values = struct.unpack_from('<%df' % (count * len(names)), data, headerEnd)
    vertices = [dict(zip(names, values[i * len(names):(i + 1) * len(names)]))
                for i in range(count)]
    return names, vertices

def _readPly(plyPath):
    """Returns (vertexCount, propertyNames, firstVertexValues) of a binary .ply file."""
    names, vertices = _readPlyVertices(plyPath)
    return len(vertices), names, tuple(vertices[0][n] for n in names)

class testUsdImportGaussianSplat(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        cls._outputDir = os.path.abspath('.')

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

    def setUp(self):
        if not self._hasMtoaGaussianSplat:
            self.skipTest("mtoa with aiGaussianSplat is not available")
        cmds.file(new=True, force=True)
        cmds.workspace(self._outputDir, openWorkspace=True)

    def _createSplatStage(self, usdFileName, halfPrecision=False, degree=0):
        usdFile = os.path.join(self._outputDir, usdFileName)
        stage = Usd.Stage.CreateNew(usdFile)
        splat = UsdVol.ParticleField3DGaussianSplat.Define(stage, '/Splat')

        count = len(_POSITIONS)
        coeffs = [(0.5, 0.25, -0.5)] * (count * (degree + 1) ** 2)
        if halfPrecision:
            splat.CreatePositionshAttr(Vt.Vec3hArray([Gf.Vec3h(*p) for p in _POSITIONS]))
            splat.CreateScaleshAttr(Vt.Vec3hArray([Gf.Vec3h(0.5, 0.5, 0.5)] * count))
            splat.CreateOrientationshAttr(Vt.QuathArray([Gf.Quath(1.0)] * count))
            splat.CreateOpacitieshAttr(Vt.HalfArray([0.5] * count))
            splat.CreateRadianceSphericalHarmonicsCoefficientshAttr(
                Vt.Vec3hArray([Gf.Vec3h(*c) for c in coeffs]))
        else:
            splat.CreatePositionsAttr(Vt.Vec3fArray(_POSITIONS))
            splat.CreateScalesAttr(Vt.Vec3fArray([(0.5, 0.5, 0.5)] * count))
            splat.CreateOrientationsAttr(Vt.QuatfArray([Gf.Quatf(1.0)] * count))
            splat.CreateOpacitiesAttr(Vt.FloatArray([0.5] * count))
            splat.CreateRadianceSphericalHarmonicsCoefficientsAttr(Vt.Vec3fArray(coeffs))
        splat.CreateRadianceSphericalHarmonicsDegreeAttr(degree)
        stage.GetRootLayer().Save()
        return usdFile

    def _importSingleSplat(self, usdFile):
        cmds.usdImport(file=usdFile, primPath='/')
        nodes = cmds.ls(type='aiGaussianSplat', long=True)
        self.assertEqual(len(nodes), 1)
        plyPath = cmds.getAttr(nodes[0] + '.filename')
        self.assertTrue(os.path.isabs(plyPath))
        self.assertTrue(os.path.isfile(plyPath))
        return nodes[0], plyPath

    def _assertPly(self, plyPath, degree):
        count, names, firstVertex = _readPly(plyPath)
        self.assertEqual(count, len(_POSITIONS))
        restCount = 3 * ((degree + 1) ** 2 - 1)
        self.assertEqual(len([n for n in names if n.startswith('f_rest_')]), restCount)
        self.assertEqual(names[:3], ['x', 'y', 'z'])
        self.assertEqual(tuple(firstVertex[:3]), _POSITIONS[0])

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testImportFloat(self):
        usdFile = self._createSplatStage("UsdImportGaussianSplatTest_Float.usda")
        node, plyPath = self._importSingleSplat(usdFile)

        self.assertEqual(node.split('|')[-1], 'SplatShape')
        self.assertEqual(os.path.basename(plyPath), 'UsdImportGaussianSplatTest_Float_Splat.ply')
        self.assertEqual(os.path.normpath(os.path.dirname(plyPath)),
            os.path.normpath(os.path.join(self._outputDir, 'data')))
        self._assertPly(plyPath, 0)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testImportHalf(self):
        usdFile = self._createSplatStage(
            "UsdImportGaussianSplatTest_Half.usda", halfPrecision=True, degree=1)
        _, plyPath = self._importSingleSplat(usdFile)
        self._assertPly(plyPath, 1)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testImportPositionsOnly(self):
        """
        Only the positions are required. The other attributes use the schema defaults, and
        the default color is degree 0 even though the degree attribute falls back to 3.
        """
        usdFile = os.path.join(self._outputDir, "UsdImportGaussianSplatTest_PositionsOnly.usda")
        stage = Usd.Stage.CreateNew(usdFile)
        splat = UsdVol.ParticleField3DGaussianSplat.Define(stage, '/Splat')
        splat.CreatePositionsAttr(Vt.Vec3fArray(_POSITIONS))
        stage.GetRootLayer().Save()

        _, plyPath = self._importSingleSplat(usdFile)
        self._assertPly(plyPath, 0)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testImportTruncatesLongerArrays(self):
        """Per-particle arrays longer than the positions are truncated, not rejected."""
        usdFile = self._createSplatStage("UsdImportGaussianSplatTest_Truncate.usda")
        stage = Usd.Stage.Open(usdFile)
        splat = UsdVol.ParticleField3DGaussianSplat(stage.GetPrimAtPath('/Splat'))
        splat.GetOpacitiesAttr().Set(Vt.FloatArray([0.5] * (len(_POSITIONS) + 2)))
        stage.GetRootLayer().Save()

        _, plyPath = self._importSingleSplat(usdFile)
        self._assertPly(plyPath, 0)

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testReimportDoesNotOverwritePly(self):
        usdFile = self._createSplatStage("UsdImportGaussianSplatTest_Reimport.usda")
        _, firstPly = self._importSingleSplat(usdFile)

        cmds.file(new=True, force=True)
        _, secondPly = self._importSingleSplat(usdFile)

        self.assertNotEqual(firstPly, secondPly)
        self.assertTrue(os.path.isfile(firstPly))
        self.assertEqual(os.path.basename(secondPly),
            'UsdImportGaussianSplatTest_Reimport_Splat_1.ply')

    def _assertRoundTrip(self, usdName, plyName, expectedFirstPosition):
        """
        The imported .ply must hold the same splats as the .ply the sample .usda was generated
        from by testSamples/3dgs/generateUsdSamples.py.
        """
        usdFile = os.path.join(_SAMPLES_DIR, 'usd', usdName)
        _, plyPath = self._importSingleSplat(usdFile)

        sourceNames, sourceVertices = _readPlyVertices(os.path.join(_SAMPLES_DIR, 'ply', plyName))
        names, vertices = _readPlyVertices(plyPath)
        self.assertEqual(len(vertices), len(sourceVertices))
        self.assertTrue(set(sourceNames).issubset(names), set(sourceNames) - set(names))
        self.assertEqual(len([n for n in names if n.startswith('f_rest_')]),
            len([n for n in sourceNames if n.startswith('f_rest_')]))

        # Values straight from the source file, so the check does not rely on the helpers above.
        for actual, expected in zip(
                (vertices[0]['x'], vertices[0]['y'], vertices[0]['z']), expectedFirstPosition):
            self.assertAlmostEqual(actual, expected, places=4)

        for i, (source, actual) in enumerate(zip(sourceVertices, vertices)):
            rot = [source['rot_%d' % j] for j in range(4)]
            length = math.sqrt(sum(r * r for r in rot))
            for name in sourceNames:
                expected = source[name]
                if name.startswith('rot_'):
                    expected /= length
                self.assertAlmostEqual(actual[name], expected, places=4,
                    msg='vertex %d, property %s' % (i, name))

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testRoundTripDegree3(self):
        self._assertRoundTrip('tennisball_degree3_10.usda', 'tennisball_degree3_binary_10.ply',
            (-1.5413243, 0.36054265, 7.3374977))

    @unittest.skipUnless(Usd.GetVersion() >= (0, 26, 5),
        "requires USD 0.26.5+ for UsdVol.ParticleField3DGaussianSplat")
    def testRoundTripDegree0(self):
        self._assertRoundTrip('tennisball_degree0_10.usda', 'tennisball_degree0_binary_10.ply',
            (-22.629562, 8.323967, -68.072632))

if __name__ == '__main__':
    unittest.main(verbosity=2)
