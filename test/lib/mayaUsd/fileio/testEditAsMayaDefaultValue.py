#!/usr/bin/env python

#
# Copyright 2023 Autodesk
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


import mayaUsd.lib
from  mayaUsd.lib import cacheToUsd

import ufe

from pxr import Usd, UsdGeom, Gf, Sdf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import fixturesUtils
import mayaUtils
import testUtils

import unittest

class EditAsMayaDefaultValueTestCase(unittest.TestCase):

    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testEditToDefaultValue(self):
        '''
        Verify that uf editing a value back to its defautl USD value, it gets properly
        written to USD after merge.
        '''
        
        usdaFile = testUtils.getTestScene('pointLight', 'pointLight.usda')
        proxyShapeDagPath, usdStage = mayaUtils.createProxyFromFile(usdaFile)
        lightUsdPathStr = '/pointLight1'
        lightUfePathStr = proxyShapeDagPath + ',' + lightUsdPathStr

        radius = usdStage.GetPrimAtPath(lightUsdPathStr).GetAttribute('inputs:radius').Get()
        self.assertAlmostEqual(radius, 1.0)

        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.canEditAsMaya(lightUfePathStr))
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(lightUfePathStr))

        lightMayaDagPathStr = 'pointLight1'
        lightMayaObj = om.MSelectionList().add(lightMayaDagPathStr).getDagPath(0).node()

        radius = cmds.getAttr('%sShape.lightRadius' % lightMayaDagPathStr)
        self.assertAlmostEqual(radius, 1.0)

        cmds.setAttr('%sShape.lightRadius' % lightMayaDagPathStr, 0.5)
        radius = cmds.getAttr('%sShape.lightRadius' % lightMayaDagPathStr)
        self.assertAlmostEqual(radius, 0.5)

        # Note: in Maya, the UI, context menu, etc go through code that use the defaults
        #       merge options which sets this.
        options = { "writeDefaults": True }
        
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.mergeToUsd(lightMayaDagPathStr, options))

        print(usdStage.GetRootLayer().ExportToString())

        radius = usdStage.GetPrimAtPath(lightUsdPathStr).GetAttribute('inputs:radius').Get()
        self.assertAlmostEqual(radius, 0.5)


if __name__ == '__main__':
    unittest.main(verbosity=2)
