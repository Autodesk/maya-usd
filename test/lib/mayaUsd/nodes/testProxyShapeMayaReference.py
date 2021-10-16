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

from maya import cmds
from maya import standalone
from pxr import Usd, Sdf

import fixturesUtils

import os
import unittest
import tempfile

import usdUtils, mayaUtils, ufeUtils

import ufe
import mayaUsd.ufe

class testProxyShapeMayaReference(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        cmds.file(new=True, force=True)

        cls.shapeNode = cmds.createNode("mayaUsdProxyShape", skipSelect=True, name="AssetShape")
        cls.shapeNode = cmds.ls(cls.shapeNode, long=True)[0]
        cls.proxyNode = cmds.listRelatives(cls.shapeNode, parent=True, fullPath=True)[0]
    
        cls.refNode = None
    
        asset_path = os.path.join(inputPath, 'ProxyShapeBaseTest', 'CubeAsset.usda')
        cmds.setAttr(cls.shapeNode + '.filePath', asset_path, type='string')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def test_LoadMayaReference(self):
        stage = mayaUsd.lib.GetPrim(self.shapeNode).GetStage()

        root_prim = stage.GetPrimAtPath('/Asset')

        variantSet = root_prim.GetVariantSet('Rig')
        variantSet.SetVariantSelection('On')

        rig_prim = stage.GetPrimAtPath('/Asset/Rig')

        self.assertTrue(rig_prim.IsValid())
        self.assertTrue(rig_prim.HasCustomDataKey('maya_associatedReferenceNode'))

        refNode = rig_prim.GetCustomDataByKey('maya_associatedReferenceNode')

        self.assertTrue(cmds.objExists(refNode))
        self.assertTrue(cmds.referenceQuery(refNode, isLoaded=True))

        associatedNodes = cmds.ls(cmds.listConnections(refNode + '.associatedNode') or [], long=True)

        self.assertTrue(self.proxyNode in associatedNodes)

    def test_UnloadMayaReference(self):
        stage = mayaUsd.lib.GetPrim(self.shapeNode).GetStage()

        root_prim = stage.GetPrimAtPath('/Asset')

        variantSet = root_prim.GetVariantSet('Rig')
        variantSet.SetVariantSelection('Off')

        rig_prim = stage.GetPrimAtPath('/Asset/Rig')
        refNode = rig_prim.GetCustomDataByKey('maya_associatedReferenceNode')

        self.assertTrue(cmds.objExists(refNode))
        self.assertFalse(cmds.referenceQuery(refNode, isLoaded=True))


if __name__ == '__main__':
    unittest.main(verbosity=2)
