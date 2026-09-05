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

import os
import unittest

import mayaUsd.lib as mayaUsdLib
import mayaUsdOptions

from pxr import Usd

from maya import cmds
from maya import standalone

import fixturesUtils


class _FrameLoggingChaser(mayaUsdLib.ExportChaser):
    name = 'TestFrameLoggingChaser'
    result = []

    def ExportDefault(self):
        _FrameLoggingChaser.result = []
        return True

    def ExportFrame(self, timeCode):
        _FrameLoggingChaser.result.append(timeCode.GetValue())
        return True


class testUsdExportExtraFrames(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        mayaUsdLib.ExportChaser.Unregister(_FrameLoggingChaser, _FrameLoggingChaser.name)

        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        mayaUsdLib.ExportChaser.Register(_FrameLoggingChaser, _FrameLoggingChaser.name)

        filePath = os.path.join(inputPath, "UsdExportFrameOffsetTest", "UsdExportFrameOffsetTest.ma")
        cmds.file(filePath, force=True, open=True)

    def _exportWithCommand(self, **kwargs):
        exportOptions = dict(
            kwargs,
            mergeTransformAndShape=True,
            chaser=_FrameLoggingChaser.name
        )
        stage = Usd.Stage.CreateInMemory()
        cmds.mayaUSDExport(file=stage.GetRootLayer().identifier, **exportOptions)
        return stage

    def _exportWithFileTranslator(self, **kwargs):
        exportOptions = dict(
            kwargs,
            mergeTransformAndShape=True,
            chaser=f'[{_FrameLoggingChaser.name}]'
        )
        stage = Usd.Stage.CreateInMemory()
        cmds.file(
            stage.GetRootLayer().identifier,
            type=fixturesUtils.exportTranslatorName(), exportAll=1,
            options=mayaUsdOptions.convertOptionsDictToText(exportOptions)
        )
        return stage

    def _assertStageTimeSamples(self, stage, expectedSamples):
        cube = stage.GetPrimAtPath('/Cube')
        attr = cube.GetAttribute('xformOp:translate')
        timeSamples = attr.GetTimeSamples()
        self.assertEqual(timeSamples, expectedSamples)

    def _assertLastExportedFrames(self, expectedFrames):
        self.assertEqual(_FrameLoggingChaser.result, expectedFrames)

    def testExtraFramesSingleFrame(self):
        """Tests that a single -extraFrame value is correctly exported."""
        stage = self._exportWithCommand(
            extraFrame=[5.0]
        )
        expectedSamples = [
            5.0
        ]
        self._assertStageTimeSamples(stage, expectedSamples)
        self._assertLastExportedFrames(expectedSamples)

    def testExtraFramesWithoutFrameRange(self):
        """Tests that -extraFrame values are exported even without -frameRange."""
        stage = self._exportWithCommand(
            extraFrame=[5.0, 6.0]
        )
        expectedSamples = [
            5.0, 6.0
        ]
        self._assertStageTimeSamples(stage, expectedSamples)
        self._assertLastExportedFrames(expectedSamples)

    def testExtraFramesWithFrameRange(self):
        """Tests that -extraFrame values are added on top of the frame range."""
        stage = self._exportWithCommand(
            extraFrame=[10.0, 11.0], frameRange=(1, 3)
        )
        expectedSamples = [
            1.0, 2.0, 3.0, 10.0, 11.0
        ]
        self._assertStageTimeSamples(stage, expectedSamples)
        self._assertLastExportedFrames(expectedSamples)

    def testExtraFramesWithFrameSamplesAndStride(self):
        """Tests -extraFrame combined with -frameSample and -frameStride."""
        stage = self._exportWithCommand(
            extraFrame=[3.0], frameRange=(1, 3), frameSample=[-0.1, 0.2], frameStride=0.5
        )
        expectedSamples = [
            0.9, 1.2, 1.4, 1.7, 1.9, 2.2, 2.4, 2.7, 2.9,
            3.0, 3.2
        ]
        self._assertStageTimeSamples(stage, expectedSamples)
        self._assertLastExportedFrames(expectedSamples)

    def testExtraFramesOverlappingEvaluatedOnce(self):
        """Tests that a -extraFrame matching an existing computed sample is only evaluated
        (and written) once, not twice."""
        stage = self._exportWithCommand(
            extraFrame=[3.2], frameRange=(1, 3), frameSample=[-0.1, 0.2]
        )
        expectedSamples = [
            0.9, 1.2, 1.9, 2.2, 2.9, 3.2
        ]
        self._assertStageTimeSamples(stage, expectedSamples)
        self._assertLastExportedFrames(expectedSamples)

    def testExtraFramesUnordered(self):
        """Tests that unordered -extraFrame values are evaluated in sorted order."""
        self._exportWithCommand(
            extraFrame=[11.0, 10.0], frameRange=(1, 3)
        )
        self._assertLastExportedFrames([
            1.0, 2.0, 3.0, 10.0, 11.0
        ])

    def testExtraFramesWithFileTranslator(self):
        """Tests that extraTimes values are supported by the export translator plugin."""
        stage = self._exportWithFileTranslator(
            extraTimes=[1.0, 2.0], animation=True
        )
        expectedSamples = [
            1.0, 2.0
        ]
        self._assertStageTimeSamples(stage, expectedSamples)
        self._assertLastExportedFrames(expectedSamples)

    def testExtraFramesNoAnimation(self):
        """Tests that extraTimes values are ignored when animation is disabled."""
        stage = self._exportWithFileTranslator(
            extraTimes=[1.0, 2.0], animation=False
        )
        self._assertStageTimeSamples(stage, [])
        self._assertLastExportedFrames([])


if __name__ == '__main__':
    unittest.main(verbosity=2)
