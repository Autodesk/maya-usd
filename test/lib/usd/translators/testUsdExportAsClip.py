#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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
import os
import unittest

from maya import cmds
from maya import standalone

from pxr import Sdf, Usd, UsdGeom, Gf, Vt, UsdUtils

import fixturesUtils

class testUsdExportAsClip(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        filePath = os.path.join(inputPath, "UsdExportAsClipTest", "UsdExportAsClipTest.ma")
        cmds.file(filePath, force=True, open=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _ValidateNumSamples(self, stage, primPath, attrName, expectedNumSamples):
        """Check that the expected number of samples have been written to a clip"""
        cube = stage.GetPrimAtPath(primPath)
        self.assertTrue(cube)
        attr = cube.GetAttribute(attrName)
        self.assertTrue(Gf.IsClose(attr.GetNumTimeSamples(), expectedNumSamples, 1e-6))

    def _ValidateSamples(self, canonicalStage, testStage, primPath, attrName, frameRange):
        """
        Check that an attibute's values between two stages are equivalent over a
        certain frame range.
        """
        def getValues(stage):
            cube = stage.GetPrimAtPath(primPath)
            self.assertTrue(cube)
            attr = cube.GetAttribute(attrName)
            return [attr.Get(time=tc) for tc in range(*frameRange)]

        for frame, x,y in zip(range(*frameRange),
                              getValues(canonicalStage),
                              getValues(testStage)):
            msg = ('different values found on frame: {frame}\n'
                   'non clip: {x}\n'
                   'clips:    {y}'.format(frame=frame, x=x, y=y))
            if isinstance(x, str):
                self.assertEqual(x, y, msg=msg)
            elif isinstance(x, Vt.Vec3fArray):
                self.assertEqual(len(x), len(y), msg)
                for xpart, ypart in zip(x,y):
                    self.assertTrue(Gf.IsClose(xpart, ypart, 1e-6), msg=msg)
            else:
                self.assertTrue(Gf.IsClose(x, y, 1e-6), msg=msg)

    def testExportAsClip(self):
        """
        Test that a maya scene exports to usd the same way if it is exported
        all at once, or in 5 frame clips and then stitched back together.
        """
        # generate clip files and validate num samples on points attribute
        clipFiles = []
        # first 5 frames have no animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.001.usda')
        clipFiles.append(usdFile)
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, frameRange=(1, 5), sss=False)
        stage = Usd.Stage.Open(usdFile)
        self._ValidateNumSamples(stage,'/world/pCube1', 'points',  1)

        # next 5 frames have no animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.005.usda')
        clipFiles.append(usdFile)
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, frameRange=(5, 10), sss=False)
        stage = Usd.Stage.Open(usdFile)
        self._ValidateNumSamples(stage, '/world/pCube1', 'points', 1)

        # next 5 frames have deformation animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.010.usda')
        clipFiles.append(usdFile)
        frames = (10, 15)
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, frameRange=frames, sss=False)
        stage = Usd.Stage.Open(usdFile)
        self._ValidateNumSamples(stage, '/world/pCube1', 'points', frames[1] + 1 - frames[0])

        # next 5 frames have no animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.015.usda')
        clipFiles.append(usdFile)
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, frameRange=(15, 20), sss=False)
        stage = Usd.Stage.Open(usdFile)
        self._ValidateNumSamples(stage, '/world/pCube1', 'points', 1)

        stitchedPath = os.path.abspath('result.usda')
        stitchedLayer = Sdf.Layer.CreateNew(stitchedPath)

        # Clip stitching behavior changed significantly between core USD 20.05
        # and 20.08. Beginning with 20.08, we need to pass an additional option
        # to ensure that authored time samples are held across gaps in value
        # clips.
        if Usd.GetVersion() > (0, 20, 5):
            self.assertTrue(
                UsdUtils.StitchClips(stitchedLayer, clipFiles, '/world',
                    startFrame=1, endFrame=20,
                    interpolateMissingClipValues=True, clipSet='default'))
        else:
            self.assertTrue(
                UsdUtils.StitchClips(stitchedLayer, clipFiles, '/world',
                    startFrame=1, endFrame=20, clipSet='default'))

        # export a non clip version for comparison
        canonicalUsdFile = os.path.abspath('canonical.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=canonicalUsdFile, frameRange=(1, 20), sss=False)

        print('comparing: \nnormal: {}\nstitched: {}'.format(canonicalUsdFile, stitchedPath))
        canonicalStage = Usd.Stage.Open(canonicalUsdFile)
        clipsStage = Usd.Stage.Open(stitchedPath)
        # visible
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube1', 'visibility', (0, 21))
        # animated visibility
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube2', 'visibility', (0, 21))
        # hidden, non animated:
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube4', 'visibility', (0, 21))
        # constant points:
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube2', 'points', (0, 21))
        # blend shape driven animated points:
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube3', 'points', (0, 21))
        # animated points:
        self._ValidateSamples(canonicalStage, clipsStage, '/world/pCube1', 'points', (0, 21))


if __name__ == '__main__':
    unittest.main(verbosity=2)
