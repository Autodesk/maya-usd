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

from pxr import UsdMaya

import mayaUsd.lib as mayaUsdLib

from maya import cmds
from maya import standalone

import os
import unittest


class testUsdMayaGetVariantSetSelections(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

        cls.usdFile = os.path.abspath('CubeWithVariantsModel.usda')
        cls.primPath = '/CubeWithVariantsModel'

    def setUp(self):
        cmds.file(new=True, force=True)

        self.assemblyNodeName = cmds.assembly(name='TestAssemblyNode',
            type='pxrUsdReferenceAssembly')
        cmds.setAttr('%s.filePath' % self.assemblyNodeName, self.usdFile, type='string')
        cmds.setAttr('%s.primPath' % self.assemblyNodeName, self.primPath, type='string')

    def _SetSelection(self, variantSetName, variantSelection):
        attrName = 'usdVariantSet_%s' % variantSetName

        if not cmds.attributeQuery(attrName, node=self.assemblyNodeName, exists=True):
            cmds.addAttr(self.assemblyNodeName, ln=attrName, dt='string', internalSet=True)

        cmds.setAttr('%s.%s' % (self.assemblyNodeName, attrName), variantSelection, type='string')

    def testNoSelections(self):
        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        self.assertEqual(variantSetSelections, {})

    def testOneSelection(self):
        self._SetSelection('modelingVariant', 'ModVariantB')

        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        self.assertEqual(variantSetSelections, {'modelingVariant': 'ModVariantB'})

    def testAllSelections(self):
        self._SetSelection('fooVariant', 'FooVariantC')
        self._SetSelection('modelingVariant', 'ModVariantB')
        self._SetSelection('shadingVariant', 'ShadVariantA')

        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        self.assertEqual(variantSetSelections,
            {'fooVariant': 'FooVariantC',
             'modelingVariant': 'ModVariantB',
             'shadingVariant': 'ShadVariantA'})

        # Verify that selecting a non-registered variant set affects the
        # stage's composition.
        prim = mayaUsdLib.GetPrim(self.assemblyNodeName)
        geomPrim = prim.GetChild('Geom')
        cubePrim = geomPrim.GetChild('Cube')

        attrValue = cubePrim.GetAttribute('variantAttribute').Get()
        self.assertEqual(attrValue, 'C')

    def testBogusVariantName(self):
        self._SetSelection('bogusVariant', 'NotARealVariantSet')

        # Invalid variantSet names should not appear in the results.
        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        self.assertEqual(variantSetSelections, {})

    def testBogusSelection(self):
        self._SetSelection('modelingVariant', 'BogusSelection')

        # Selections are NOT validated, so any "selection" for a valid
        # variantSet should appear in the results.
        variantSetSelections = UsdMaya.GetVariantSetSelections(self.assemblyNodeName)
        self.assertEqual(variantSetSelections, {'modelingVariant': 'BogusSelection'})


if __name__ == '__main__':
    unittest.main(verbosity=2)
