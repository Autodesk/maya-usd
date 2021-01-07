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

# mayaUsdLib is not used in the test, but we import it here to force the
# library to load so that the VP2_RENDER_DELEGATE_PROXY TfEnvSetting gets
# registered.
from mayaUsd import lib as mayaUsdLib

from pxr import Tf
from pxr import Usd

from maya import cmds
from maya import standalone

import fixturesUtils

import os
import unittest


@unittest.skipIf(Tf.GetEnvSetting('VP2_RENDER_DELEGATE_PROXY'),
    'Not applicable when using the Viewport 2.0 render deleggate')
class testHdImagingShape(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        mayaSceneFilePath = os.path.join(inputPath, 'HdImagingShapeTest',
            'HdImagingShapeTest.ma')

        cmds.file(mayaSceneFilePath, open=True, force=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testAutomaticImagingShapeCreation(self):
        """
        Tests that the pxrHdImagingShape is created automatically for a USD
        proxy shape when *not* using the Viewport 2.0 render delegate.

        The Maya scene file itself does not contain a pxrHdImagingShape.
        """
        # Sanity check that the USD proxy shape exists.
        proxyShapePath = '|CubeProxy|CubeProxyShape'
        self.assertTrue(cmds.objExists(proxyShapePath))
        self.assertEqual(cmds.nodeType(proxyShapePath), 'mayaUsdProxyShape')

        # Make sure the pxrHdImagingShape (and its parent transform) exist.
        hdImagingTransformPath = '|HdImaging'
        hdImagingShapePath = '%s|HdImagingShape' % hdImagingTransformPath

        self.assertTrue(cmds.objExists(hdImagingTransformPath))
        self.assertEqual(cmds.nodeType(hdImagingTransformPath), 'transform')

        self.assertTrue(cmds.objExists(hdImagingShapePath))
        self.assertEqual(cmds.nodeType(hdImagingShapePath), 'pxrHdImagingShape')

        self.assertNotEqual(
            cmds.ls(hdImagingTransformPath, uuid=True),
            cmds.ls(hdImagingShapePath, uuid=True))

    def testImagingShapeDoesNotExport(self):
        """
        Tests that the USD exported from a scene that uses the
        pxrHdImagingShape does *not* include any prims for the
        pxrHdImagingShape itself.

        The pxrHdImagingShape and its parent transform are set so that they do
        not write to the Maya scene file and are not exported to USD.
        """
        usdFilePath = os.path.abspath('ProxyShapeBaseExportTest.usda')
        cmds.mayaUSDExport(file=usdFilePath)

        usdStage = Usd.Stage.Open(usdFilePath)
        prim = usdStage.GetPrimAtPath('/HdImaging')
        self.assertFalse(prim)
        prim = usdStage.GetPrimAtPath('/HdImaging/HdImagingShape')
        self.assertFalse(prim)

    def testRootNodeReordering(self):
        """
        Tests that root-level Maya nodes can still be reordered when the
        pxrHdImagingShape is present.

        At one time, the pxrHdImagingShape and its transform were being locked,
        which ended up preventing other root-level nodes from being reordered.
        """
        cmds.polyCube(n='testNode1')
        cmds.polyCube(n='testNode2')
        cmds.reorder('testNode1', back=True)
        cmds.reorder('testNode2', front=True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
