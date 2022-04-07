#!/usr/bin/env python

#
# Copyright 2022 Autodesk
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

import fixturesUtils
import mayaUtils
import testUtils
import ufeUtils
import usdUtils

from pxr import Gf

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as om

import ufe

from functools import partial
import unittest

class LightTestCase(unittest.TestCase):
    '''Verify the Light UFE translate interface, for multiple runtimes.
    
    UFE Feature : Light
    Maya Feature : light
    Action : set & read light parameters
    Applied On Selection : No
    Undo/Redo Test : No
    Expect Results To Test :
        - Setting a value through Ufe correctly updates USD
        - Reading a value through Ufe gets the correct value
    Edge Cases :
        - None.
    '''

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
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # value from UsdGeomLinearUnits
        self.inchesToCm = 2.54
        self.mmToCm = 0.1

    def _StartTest(self, testName):
        cmds.file(force=True, new=True)
        mayaUtils.loadPlugin("mayaUsdPlugin")
        self._testName = testName
        testFile = testUtils.getTestScene("light", self._testName + ".usda")
        mayaUtils.createProxyFromFile(testFile)
        globalSelection = ufe.GlobalSelection.get()
        globalSelection.clear()

    def _TestLightAttributes(self, ufeLight, usdLight):
        # Trust that the USD API works correctly, validate that UFE gives us
        # the same answers
        self._TestIntensity(ufeLight, usdLight)
        self._TestDiffuse(ufeLight, usdLight)
        self._TestSpecular(ufeLight, usdLight)
        self._TestShadowEnable(ufeLight, usdLight)
        self._TestColor(ufeLight, usdLight)
        self._TestShadowColor(ufeLight, usdLight)
        self._TestSphereProps(ufeLight, usdLight)
        self._TestConeProps(ufeLight, usdLight)

    def _TestIntensity(self, ufeLight, usdLight):
        usdAttr = usdLight.GetAttribute('inputs:intensity')
        self.assertAlmostEqual(usdAttr.Get(), ufeLight.intensity())

        usdAttr.Set(0.5)
        self.assertAlmostEqual(0.5, ufeLight.intensity())

        ufeLight.intensity(100)
        self.assertAlmostEqual(usdAttr.Get(), 100)

    def _TestDiffuse(self, ufeLight, usdLight):
        usdAttr = usdLight.GetAttribute('inputs:diffuse')
        self.assertAlmostEqual(usdAttr.Get(), ufeLight.diffuse())

        usdAttr.Set(0.5)
        self.assertAlmostEqual(0.5, ufeLight.diffuse())

        ufeLight.diffuse(0.9)
        self.assertAlmostEqual(usdAttr.Get(), 0.9)

    def _TestSpecular(self, ufeLight, usdLight):
        usdAttr = usdLight.GetAttribute('inputs:specular')
        self.assertAlmostEqual(usdAttr.Get(), ufeLight.specular())

        usdAttr.Set(0.5)
        self.assertAlmostEqual(0.5, ufeLight.specular())

        ufeLight.specular(0.9)
        self.assertAlmostEqual(usdAttr.Get(), 0.9)

    def _TestShadowEnable(self, ufeLight, usdLight):
        usdAttr = usdLight.GetAttribute('inputs:shadow:enable')
        self.assertEqual(usdAttr.Get(), ufeLight.shadowEnable())

        usdAttr.Set(False)
        self.assertEqual(False, ufeLight.shadowEnable())

        ufeLight.shadowEnable(True)
        self.assertAlmostEqual(usdAttr.Get(), True)        

    def _TestColor(self, ufeLight, usdLight):
        usdAttr = usdLight.GetAttribute('inputs:color')
        self.assertAlmostEqual(usdAttr.Get()[0], ufeLight.color().r())
        self.assertAlmostEqual(usdAttr.Get()[1], ufeLight.color().g())
        self.assertAlmostEqual(usdAttr.Get()[2], ufeLight.color().b())

    def _TestShadowColor(self, ufeLight, usdLight):
        usdAttr = usdLight.GetAttribute('inputs:shadow:color')
        self.assertAlmostEqual(usdAttr.Get()[0], ufeLight.shadowColor().r())
        self.assertAlmostEqual(usdAttr.Get()[1], ufeLight.shadowColor().g())
        self.assertAlmostEqual(usdAttr.Get()[2], ufeLight.shadowColor().b())

    def _TestSphereProps(self, ufeLight, usdLight):
        usdAttr = usdLight.GetAttribute('inputs:radius')
        self.assertAlmostEqual(usdAttr.Get(), ufeLight.sphereInterface().sphereProps().radius)
        self.assertEqual(usdAttr.Get() == 0, ufeLight.sphereInterface().sphereProps().asPoint)

    def _TestConeProps(self, ufeLight, usdLight):
        usdAttrFocus = usdLight.GetAttribute('inputs:shaping:focus')
        usdAttrAngle = usdLight.GetAttribute('inputs:shaping:cone:angle')
        usdAttrSoftness = usdLight.GetAttribute('inputs:shaping:cone:softness')
        self.assertAlmostEqual(usdAttrFocus.Get(), ufeLight.coneInterface().coneProps().focus)
        self.assertAlmostEqual(usdAttrAngle.Get(), ufeLight.coneInterface().coneProps().angle)
        self.assertAlmostEqual(usdAttrSoftness.Get(), ufeLight.coneInterface().coneProps().softness)

    def testUsdLight(self):
        self._StartTest('SimpleLight')
        mayaPathSegment = mayaUtils.createUfePathSegment('|stage|stageShape')
        lightUsdPathSegment = usdUtils.createUfePathSegment('/lights/simpleLight')
        lightPath = ufe.Path([mayaPathSegment, lightUsdPathSegment])
        lightItem = ufe.Hierarchy.createItem(lightPath)

        ufeLight = ufe.Light.light(lightItem)
        usdLight = usdUtils.getPrimFromSceneItem(lightItem)
        self.assertEqual(ufeLight.type(), ufe.Light.Spot)
        self._TestLightAttributes(ufeLight, usdLight)


if __name__ == '__main__':
    unittest.main(verbosity=2)
