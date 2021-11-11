#!/pxrpythonsubst

#
# Copyright 2020 Apple
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

from __future__ import division

import fixturesUtils
import logging
import os
import unittest
from maya import cmds, standalone
from pxr import Usd


class testUsdImportFramerates(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.input_dir = fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')
        cls.usd_file = os.path.join(cls.input_dir, "UsdImportFramerates/framerate.usda")
        cls.stage = Usd.Stage.Open(cls.usd_file)
        cls.base_rate = cls.stage.GetTimeCodesPerSecond()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def getKeyframes(cls, path):
        """Get all keyframe values on a prims attributes"""
        frames = set()
        prim = cls.stage.GetPrimAtPath(path)
        attrs = prim.GetAuthoredAttributes()

        for attr in attrs:
            samples = attr.GetTimeSamples()
            frames.update(samples)

        return sorted(frames)

    def test_framerate_imports(self):
        """Test that the USD's timesamples are appropriately
        converted to match the Mayas uiUnits"""

        # Only check a few of these since the logic should
        # hold up the same no matter what, but
        fps_map = {
            "ntsc": 30,
            "film": 24,
            "ntscf": 60
        }

        base_rate = self.stage.GetTimeCodesPerSecond()
        start = self.stage.GetStartTimeCode()
        end = self.stage.GetEndTimeCode()

        nodes = {
            'Root|joint1': self.getKeyframes("/Root/Skin"),
            'Root|Mesh': self.getKeyframes("/Root/Mesh"),
            "blendShape1": self.getKeyframes("/Root/Skin"),  # Mesh Anim
            "Cam": self.getKeyframes("/Root/Cam"),
            # For some reason the clip planes don't identify keys on the Cam Shape directly
            "CamShape_nearClipPlane": self.getKeyframes("/Root/Cam"),
            "blendShape2": self.getKeyframes("/Root/Nurbs")  # Patch anim
        }

        # TODO: Add NurbsCurves to the nodes list when the importer supports it

        for name, fps in fps_map.items():
            cmds.file(new=True, force=True)
            cmds.currentUnit(time=name)
            cmds.playbackOptions(minTime=10)
            cmds.playbackOptions(maxTime=20)

            time_scale = (fps / base_rate)

            cmds.mayaUSDImport(f=self.usd_file, readAnimData=True)

            # First make sure the playback slider is set correctly
            self.assertAlmostEquals(start * time_scale, cmds.playbackOptions(query=True, minTime=True))
            self.assertAlmostEquals(end * time_scale, cmds.playbackOptions(query=True, maxTime=True))

            for node, frames in nodes.items():
                mframes = sorted(set(cmds.keyframe(node, query=True)))

                numMFrames = len(mframes)
                numFrames = len(frames)
                self.assertTrue(numFrames == numMFrames,
                                "A mismatched number of frames was found {}:{}".format(numFrames, numMFrames))

                for idx, frame in enumerate(frames):
                    self.assertAlmostEquals(frame * time_scale, mframes[idx])


if __name__ == '__main__':
    unittest.main(verbosity=2)
