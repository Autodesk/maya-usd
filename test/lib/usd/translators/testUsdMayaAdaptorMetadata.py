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

import mayaUsd.lib as mayaUsdLib

from pxr import Usd
from pxr import UsdGeom

from maya import cmds
from maya import standalone

import os
import unittest

import fixturesUtils


class testUsdMayaAdaptorMetadata(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

        cls.inputUsdFile = os.path.join(cls.inputPath,
            'UsdMayaAdaptorMetadataTest', 'UsdAttrs.usda')

    def testImport_HiddenInstanceableKind(self):
        """
        Tests that the adaptor mechanism can import
        hidden, instanceable, and kind metadata properly.
        """
        cmds.file(new=True, force=True)
        cmds.usdImport(file=self.inputUsdFile, shadingMode=[["none", "default"], ])

        # pCube1 and pCube2 have USD_kind.
        self.assertEqual(cmds.getAttr('pCube1.USD_kind'), 'potato')
        self.assertEqual(cmds.getAttr('pCube2.USD_kind'), 'bakedpotato')
        
        # pCube2, pCube4, and pCube5 have USD_hidden. pCube1 and pCube3 do not.
        self.assertTrue(
            cmds.attributeQuery('USD_hidden', node='pCube2', exists=True))
        self.assertTrue(
            cmds.attributeQuery('USD_hidden', node='pCube4', exists=True))
        self.assertTrue(
            cmds.attributeQuery('USD_hidden', node='pCube5', exists=True))
        self.assertFalse(
            cmds.attributeQuery('USD_hidden', node='pCube1', exists=True))
        self.assertFalse(
            cmds.attributeQuery('USD_hidden', node='pCube3', exists=True))

        self.assertTrue(cmds.getAttr('pCube2.USD_hidden'))
        self.assertTrue(cmds.getAttr('pCube4.USD_hidden'))
        self.assertFalse(cmds.getAttr('pCube5.USD_hidden'))
        
        # pCube3 and pCube4 have USD_instanceable.
        self.assertTrue(cmds.getAttr('pCube3.USD_instanceable'))
        self.assertTrue(cmds.getAttr('pCube4.USD_instanceable'))

    def testImport_HiddenInstanceable(self):
        """Tests import with only hidden, instanceable metadata and not kind."""
        cmds.file(new=True, force=True)
        cmds.usdImport(file=self.inputUsdFile, 
                shadingMode=[["none", "default"], ],
                metadata=['hidden', 'instanceable'])

        # pCube1 and pCube2 have USD_kind, but that shouldn't be imported.
        self.assertFalse(
            cmds.attributeQuery('USD_kind', node='pCube1', exists=True))
        self.assertFalse(
            cmds.attributeQuery('USD_kind', node='pCube2', exists=True))
        
        # hidden should be imported.
        self.assertTrue(
            cmds.attributeQuery('USD_hidden', node='pCube2', exists=True))
        self.assertTrue(cmds.getAttr('pCube2.USD_hidden'))
        
        # instanceable should be imported.
        self.assertTrue(
            cmds.attributeQuery('USD_instanceable', node='pCube3', exists=True))
        self.assertTrue(cmds.getAttr('pCube3.USD_instanceable'))

    def testImport_TypeName(self):
        """Tests import with metadata=typeName manually specified."""
        cmds.file(new=True, force=True)
        cmds.usdImport(file=self.inputUsdFile,
                shadingMode=[["none", "default"], ],
                metadata=["typeName"])

        # typeName metadata should come through.
        self.assertTrue(
            cmds.attributeQuery('USD_typeName', node='pCube1', exists=True))
        self.assertEqual(cmds.getAttr('pCube1.USD_typeName'), 'Mesh')
        self.assertTrue(
            cmds.attributeQuery('USD_typeName', node='World', exists=True))
        self.assertEqual(cmds.getAttr('World.USD_typeName'), 'Xform')

        # No USD_kind, hidden, or instanceable.
        self.assertFalse(
            cmds.attributeQuery('USD_kind', node='pCube1', exists=True))
        self.assertFalse(
            cmds.attributeQuery('USD_hidden', node='pCube2', exists=True))
        self.assertFalse(
            cmds.attributeQuery('USD_instanceable', node='pCube3', exists=True))

    def testImport_GeomModelAPI(self):
        """Tests importing UsdGeomModelAPI attributes."""
        cmds.file(new=True, force=True)
        cmds.usdImport(file=self.inputUsdFile,
                shadingMode=[["none", "default"], ],
                apiSchema=['GeomModelAPI'])

        worldProxy = mayaUsdLib.Adaptor('World')
        modelAPI = worldProxy.GetSchema(UsdGeom.ModelAPI)

        # model:cardGeometry
        self.assertTrue(
            cmds.attributeQuery('USD_ATTR_model_cardGeometry',
            node='World', exists=True))
        self.assertEqual(cmds.getAttr('World.USD_ATTR_model_cardGeometry'),
            UsdGeom.Tokens.fromTexture)
        self.assertEqual(
            modelAPI.GetAttribute(UsdGeom.Tokens.modelCardGeometry).Get(),
            UsdGeom.Tokens.fromTexture)

        # model:cardTextureXPos
        self.assertTrue(
            cmds.attributeQuery('USD_ATTR_model_cardTextureXPos',
            node='World', exists=True))
        self.assertEqual(cmds.getAttr('World.USD_ATTR_model_cardTextureXPos'),
            'right.png')
        self.assertEqual(
            modelAPI.GetAttribute(UsdGeom.Tokens.modelCardTextureXPos).Get(),
            'right.png')

    def testExport(self):
        """
        Tests that the adaptor mechanism can export
        USD_hidden, USD_instanceable, and USD_kind attributes by setting
        the correct metadata in the output USD file.
        """
        cmds.file(new=True, force=True)
        cmds.usdImport(file=self.inputUsdFile, shadingMode=[["none", "default"], ])

        newUsdFilePath = os.path.abspath('UsdAttrsNew.usda')
        cmds.usdExport(file=newUsdFilePath, shadingMode='none')
        newUsdStage = Usd.Stage.Open(newUsdFilePath)
        
        # pCube1 and pCube2 have USD_kind.
        prim1 = newUsdStage.GetPrimAtPath('/World/pCube1')
        self.assertEqual(Usd.ModelAPI(prim1).GetKind(), 'potato')
        prim2 = newUsdStage.GetPrimAtPath('/World/pCube2')
        self.assertEqual(Usd.ModelAPI(prim2).GetKind(), 'bakedpotato')
        
        # pCube2, pCube4, and pCube5 have USD_hidden. pCube1 and pCube3 do not.
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube2').HasAuthoredHidden())
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube4').HasAuthoredHidden())
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube5').HasAuthoredHidden())
        self.assertFalse(newUsdStage.GetPrimAtPath('/World/pCube1').HasAuthoredHidden())
        self.assertFalse(newUsdStage.GetPrimAtPath('/World/pCube3').HasAuthoredHidden())

        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube2').IsHidden())
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube4').IsHidden())
        self.assertFalse(newUsdStage.GetPrimAtPath('/World/pCube5').IsHidden())
        
        # pCube3 and pCube4 have USD_instanceable.
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube3').IsInstanceable())
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube4').IsInstanceable())

    def testExport_GeomModelAPI(self):
        """Tests export with the GeomModelAPI."""
        cmds.file(new=True, force=True)
        cmds.usdImport(file=self.inputUsdFile,
                shadingMode=[["none", "default"], ],
                apiSchema=['GeomModelAPI'])

        newUsdFilePath = os.path.abspath('UsdAttrsNew_GeomModelAPI.usda')
        cmds.usdExport(file=newUsdFilePath, shadingMode='none')
        newUsdStage = Usd.Stage.Open(newUsdFilePath)

        world = newUsdStage.GetPrimAtPath('/World')
        self.assertEqual(world.GetAppliedSchemas(), ['GeomModelAPI'])

        geomModelAPI = UsdGeom.ModelAPI(world)
        self.assertEqual(geomModelAPI.GetModelCardTextureXPosAttr().Get(),
                'right.png')

    def testExport_GeomModelAPI_MotionAPI(self):
        """Tests export with both the GeomModelAPI and MotionAPI."""
        cmds.file(new=True, force=True)
        cmds.usdImport(file=self.inputUsdFile,
                shadingMode=[["none", "default"], ],
                apiSchema=['GeomModelAPI', 'MotionAPI'])

        newUsdFilePath = os.path.abspath('UsdAttrsNew_TwoAPIs.usda')
        # usdExport used to export all API schemas found as dynamic attributes. We now
        # require the list to be explicit, mirroring the way usdImport works.
        cmds.usdExport(file=newUsdFilePath, shadingMode='none',
                       apiSchema=['GeomModelAPI', 'MotionAPI'])
        newUsdStage = Usd.Stage.Open(newUsdFilePath)

        world = newUsdStage.GetPrimAtPath('/World')
        self.assertEqual(world.GetAppliedSchemas(),
                ['MotionAPI', 'GeomModelAPI'])

        geomModelAPI = UsdGeom.ModelAPI(world)
        self.assertEqual(geomModelAPI.GetModelCardTextureXPosAttr().Get(),
                'right.png')

        motionAPI = UsdGeom.MotionAPI(world)
        self.assertAlmostEqual(motionAPI.GetVelocityScaleAttr().Get(), 4.2,
                places=6)

if __name__ == '__main__':
    unittest.main(verbosity=2)
