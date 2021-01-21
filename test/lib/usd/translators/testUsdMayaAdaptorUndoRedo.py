#!/usr/bin/env mayapy
#
# Copyright 2020 Pixar
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

import mayaUsd.lib as mayaUsdLib

from pxr import UsdGeom

from maya import cmds
from maya import standalone

import unittest

import fixturesUtils


class testUsdMayaAdaptorUndoRedo(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # We load the plugin here to ensure that the usdUndoHelperCmd has been
        # registered. See the documentation on UsdMayaAdaptor for more detail.
        cls.inputPath = fixturesUtils.setUpClass(__file__, loadPlugin=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testUndoRedo(self):
        """Tests that adaptors work with undo/redo."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)
        adaptor = mayaUsdLib.Adaptor("group1")
        self.assertEqual(adaptor.GetAppliedSchemas(), [])

        # Do a single operation, then undo, then redo.
        adaptor.ApplySchema(UsdGeom.ModelAPI)
        self.assertEqual(adaptor.GetAppliedSchemas(), ["GeomModelAPI"])
        cmds.undo()
        self.assertEqual(adaptor.GetAppliedSchemas(), [])
        cmds.redo()
        self.assertEqual(adaptor.GetAppliedSchemas(), ["GeomModelAPI"])

        # Do a compound operation, then undo, then redo.
        cmds.undoInfo(openChunk=True)
        adaptor.ApplySchema(UsdGeom.MotionAPI).CreateAttribute(
                UsdGeom.Tokens.motionVelocityScale).Set(0.42)
        self.assertEqual(adaptor.GetAppliedSchemas(),
                ["GeomModelAPI", "MotionAPI"])
        self.assertAlmostEqual(adaptor.GetSchema(UsdGeom.MotionAPI).GetAttribute(
                UsdGeom.Tokens.motionVelocityScale).Get(), 0.42)
        cmds.undoInfo(closeChunk=True)
        cmds.undo()
        self.assertEqual(adaptor.GetAppliedSchemas(), ["GeomModelAPI"])
        self.assertFalse(adaptor.GetSchema(UsdGeom.MotionAPI).GetAttribute(
                UsdGeom.Tokens.motionVelocityScale))
        self.assertIsNone(adaptor.GetSchema(UsdGeom.MotionAPI).GetAttribute(
                UsdGeom.Tokens.motionVelocityScale).Get())
        cmds.redo()
        self.assertEqual(adaptor.GetAppliedSchemas(),
                ["GeomModelAPI", "MotionAPI"])
        self.assertAlmostEqual(adaptor.GetSchema(UsdGeom.MotionAPI).GetAttribute(
                UsdGeom.Tokens.motionVelocityScale).Get(), 0.42)


if __name__ == '__main__':
    unittest.main(verbosity=2)
