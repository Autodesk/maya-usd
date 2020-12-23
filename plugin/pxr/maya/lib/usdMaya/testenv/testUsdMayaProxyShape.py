#!/usr/bin/env mayapy
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

from pxr import Usd, Tf

from maya import cmds
from maya import standalone

import os
import unittest


class testUsdMayaProxyShape(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testProxyShapeBoundingBox(self):
        mayaFile = os.path.abspath('ProxyShape.ma')
        cmds.file(mayaFile, open=True, force=True)

        # Verify that the proxy shape read something from the USD file.
        bboxSize = cmds.getAttr('Cube_usd.boundingBoxSize')[0]
        self.assertEqual(bboxSize, (1.0, 1.0, 1.0))

        # The VP2 render delegate doesn't use additional proxy shape
        if not Tf.GetEnvSetting('VP2_RENDER_DELEGATE_PROXY'):
            # The proxy shape is imaged by the pxrHdImagingShape, which should be
            # created by the proxy shape's postConstructor() method. Make sure the
            # pxrHdImagingShape (and its parent transform) exist.
            hdImagingTransformPath = '|HdImaging'
            hdImagingShapePath = '%s|HdImagingShape' % hdImagingTransformPath

            self.assertTrue(cmds.objExists(hdImagingTransformPath))
            self.assertEqual(cmds.nodeType(hdImagingTransformPath), 'transform')

            self.assertTrue(cmds.objExists(hdImagingShapePath))
            self.assertEqual(cmds.nodeType(hdImagingShapePath), 'pxrHdImagingShape')

            self.assertNotEqual(
                    cmds.ls(hdImagingTransformPath, uuid=True),
                    cmds.ls(hdImagingShapePath, uuid=True))

        # The pxrHdImagingShape and its parent transform are set so that they
        # do not write to the Maya scene file and are not exported by
        # usdExport, so do a test export and make sure that's the case.
        usdFilePath = os.path.abspath('ProxyShapeExportTest.usda')
        cmds.usdExport(file=usdFilePath)

        usdStage = Usd.Stage.Open(usdFilePath)
        prim = usdStage.GetPrimAtPath('/HdImaging')
        self.assertFalse(prim)
        prim = usdStage.GetPrimAtPath('/HdImaging/HdImagingShape')
        self.assertFalse(prim)

        # Make sure that we can reorder root nodes in the scene with the
        # pxrHdImagingShape present.
        cmds.polyCube(n="testNode1")
        cmds.polyCube(n="testNode2")
        cmds.reorder("testNode1", back=True)
        cmds.reorder("testNode2", front=True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
