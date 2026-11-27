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

'''
Unit tests of usdUfe.extractStageEdits(), on USD stages without Maya scenes.

Each test edits a stage in memory, extracts its edits, then discards the edits of the
layers in memory by reloading them from disk: opening the extracted layer must then
give the same values as the edited stage.
'''

from pxr import Sdf, Usd, UsdGeom

import fixturesUtils

import os
import shutil
import unittest

import usdUfe


class testUtilsStageEdits(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__, initializeStandalone=False)
        cls.sourceDataDir = os.path.join(
            inputPath, '..', '..', 'usd', 'translators', 'UsdExportStageEditsTest')

    def setUp(self):
        # Each test uses its own copy of the files: the layers stay in memory.
        self.dataDir = os.path.abspath('data_%s' % self._testMethodName)
        if os.path.exists(self.dataDir):
            shutil.rmtree(self.dataDir)
        shutil.copytree(self.sourceDataDir, self.dataDir)
        self.stage = Usd.Stage.Open(os.path.join(self.dataDir, 'root.usda'))
        self.editsPath = os.path.join(self.dataDir, 'out', 'edits.usda')

    def layer(self, fileName):
        return Sdf.Layer.Find(os.path.join(self.dataDir, fileName))

    def extract(self):
        '''Extract the edits of the stage, discard them from the layers in memory, and
        return the layer of the edits and the stage opened from it.'''
        edits = usdUfe.extractStageEdits(self.stage, self.editsPath)
        self.assertTrue(edits)
        self.assertTrue(edits.Export(self.editsPath))

        # (Reloading a layer can release other layers: collect them first.)
        dirtyLayers = [layer for layer in self.stage.GetUsedLayers()
                       if not layer.anonymous and layer.dirty]
        for layer in dirtyLayers:
            layer.Reload(True)
        self.stage.GetSessionLayer().Clear()

        return Sdf.Layer.FindOrOpen(self.editsPath), Usd.Stage.Open(self.editsPath)

    def assertSameValues(self, attributes, times=(None,)):
        '''Extract the edits and verify that the attributes have the same values in the
        stage opened from the edits as in the edited stage.'''
        def values(stage):
            result = {}
            for path in attributes:
                attr = stage.GetAttributeAtPath(path)
                result[path] = [attr.Get(Usd.TimeCode.Default() if t is None else t) if attr
                                else None for t in times]
            return result

        expected = values(self.stage)
        layer, extracted = self.extract()
        actual = values(extracted)
        for path in attributes:
            for time, a, e in zip(times, actual[path], expected[path]):
                if isinstance(e, float):
                    self.assertAlmostEqual(a, e, msg='%s at %s' % (path, time))
                else:
                    self.assertEqual(a, e, '%s at %s' % (path, time))
        return layer, extracted

    def testCleanStage(self):
        '''A stage without edits has nothing to extract.'''
        self.assertIsNone(usdUfe.extractStageEdits(self.stage, self.editsPath))

    def testOnlyEditedFields(self):
        '''Only the edited fields are extracted, above the original files.'''
        with Usd.EditContext(self.stage, self.stage.GetRootLayer()):
            UsdGeom.XformCommonAPI(self.stage.GetPrimAtPath('/Top/Cube1')).SetTranslate((5, 6, 7))
            UsdGeom.Sphere.Define(self.stage, '/Top/NewSphere').CreateRadiusAttr(2.0)

        layer, _ = self.assertSameValues(['/Top/Cube1.xformOp:translate', '/Top/NewSphere.radius'])

        self.assertEqual(layer.subLayerPaths[0], self.layer('root.usda').identifier)
        self.assertEqual(layer.defaultPrim, 'Top')
        self.assertTrue(layer.GetAttributeAtPath('/Top/Cube1.xformOp:translate'))
        self.assertFalse(layer.GetAttributeAtPath('/Top/Cube1.xformOpOrder'))
        self.assertFalse(layer.GetAttributeAtPath('/Top/Cube1.sharedAttr'))
        self.assertFalse(layer.GetPrimAtPath('/Top/Ref'))

    def testSessionLayer(self):
        '''The opinions of the session layer are extracted.'''
        with Usd.EditContext(self.stage, self.stage.GetSessionLayer()):
            self.stage.GetAttributeAtPath('/Top/Cube1.sharedAttr').Set(99.0)

        self.assertSameValues(['/Top/Cube1.sharedAttr'])

    def testRemovedOpinions(self):
        '''
        The opinions removed from a layer are removed from the result: the weaker value
        is extracted, the value is blocked, or the prim is deactivated.
        '''
        rootLayer = self.stage.GetRootLayer()
        rootLayer.GetAttributeAtPath('/Top/Cube1.removable').ClearDefaultValue()
        rootLayer.GetAttributeAtPath('/Top/Cube1.blockable').ClearDefaultValue()
        with Usd.EditContext(self.stage, rootLayer):
            self.stage.RemovePrim('/Top/Camera1')

        _, extracted = self.assertSameValues(['/Top/Cube1.removable', '/Top/Cube1.blockable'])
        self.assertFalse(extracted.GetPrimAtPath('/Top/Camera1').IsActive())

    def testWeakerLayerEdits(self):
        '''
        The edits of a sublayer stay hidden by the stronger layers: values, explicit
        and prepended list ops, overs on a def.
        '''
        with Usd.EditContext(self.stage, self.layer('sub.usda')):
            cube = self.stage.GetPrimAtPath('/Top/Cube1')
            cube.GetAttribute('sharedAttr').Set(20.0)
            cube.GetAttribute('weakAttr').Set(30.0)
            cube.GetRelationship('explicitTargets').SetTargets([Sdf.Path('/Top/Cone1')])
            cube.GetRelationship('prependedTargets').RemoveTarget(Sdf.Path('/Top/Cylinder1'))
            self.stage.DefinePrim('/Top/DefinedInSub', 'Xform')

        expectedTargets = {name: self.stage.GetPrimAtPath('/Top/Cube1').GetRelationship(name)
                           .GetTargets() for name in ['explicitTargets', 'prependedTargets']}

        _, extracted = self.assertSameValues(['/Top/Cube1.sharedAttr', '/Top/Cube1.weakAttr'])
        cube = extracted.GetPrimAtPath('/Top/Cube1')
        for name, targets in expectedTargets.items():
            self.assertEqual(cube.GetRelationship(name).GetTargets(), targets, name)
        self.assertTrue(extracted.GetPrimAtPath('/Top/DefinedInSub').IsDefined())

    def testTimeOffsetsAndTcps(self):
        '''
        The time samples of a sublayer with an offset and a scale, and a reference
        authored in a sublayer with another TCPS than the stage, keep their timing.
        '''
        heightPath = Sdf.Path('/Top/Cylinder1.height')
        for time, value in [(10.0, 5.0), (20.0, 9.0)]:
            self.layer('anim.usda').SetTimeSample(heightPath, time, value)
        with Usd.EditContext(self.stage, self.layer('anim48.usda')):
            self.stage.OverridePrim('/Top/Anim48Ref').GetReferences().AddReference('./asset48.usda')

        self.assertSameValues(['/Top/Cylinder1.height', '/Top/Anim48Ref.value'],
                              times=(0, 10, 12, 20, 24, 30, 48))

    def testVariantSelections(self):
        '''
        Variant selections are composed variant set by variant set: a selection in a
        weaker layer is kept, and a cleared selection is blocked.
        '''
        with Usd.EditContext(self.stage, self.layer('sub.usda')):
            self.stage.GetPrimAtPath('/Top/Cone1').GetVariantSet('color').SetVariantSelection('blue')
        with Usd.EditContext(self.stage, self.stage.GetRootLayer()):
            self.stage.GetPrimAtPath('/Top/Cone1').GetVariantSet('size').ClearVariantSelection()
        cone = self.stage.GetPrimAtPath('/Top/Cone1')
        self.assertEqual(cone.GetVariantSet('size').GetVariantSelection(), '')

        _, extracted = self.assertSameValues(['/Top/Cone1.colorValue', '/Top/Cone1.radius'])
        cone = extracted.GetPrimAtPath('/Top/Cone1')
        self.assertEqual(cone.GetVariantSet('size').GetVariantSelection(), '')
        self.assertEqual(cone.GetVariantSet('color').GetVariantSelection(), 'blue')

    def testAssetPaths(self):
        '''Relative asset paths, with and without "./", still point at the original files.'''
        with Usd.EditContext(self.stage, self.stage.GetRootLayer()):
            cube = self.stage.GetPrimAtPath('/Top/Cube1')
            cube.GetAttribute('texFile').Set(Sdf.AssetPath('./textures/other.png'))
            cube.GetAttribute('searchTex').Set(Sdf.AssetPath('textures/other.png'))

        _, extracted = self.extract()
        expected = os.path.normcase(os.path.join(self.dataDir, 'textures', 'other.png'))
        for name in ['texFile', 'searchTex']:
            resolved = extracted.GetAttributeAtPath('/Top/Cube1.' + name).Get().resolvedPath
            self.assertEqual(os.path.normcase(os.path.normpath(resolved)), expected, name)

    def testMutedSublayer(self):
        '''The opinions of a muted sublayer are removed, without copying its parent.'''
        self.stage.MuteLayer(self.layer('sub.usda').identifier)

        layer, extracted = self.assertSameValues(['/Top/Cube1.weakAttr', '/Top/Cube1.removable'])
        self.assertFalse(layer.GetPrimAtPath('/Top/Camera1'))

    def testAnonymousSublayers(self):
        '''
        The opinions of anonymous sublayers, with anonymous sublayers of their own, are
        extracted in their strength order.
        '''
        parent = Sdf.Layer.CreateAnonymous('parent')
        child = Sdf.Layer.CreateAnonymous('child')
        parent.subLayerPaths.append(child.identifier)
        self.stage.GetRootLayer().subLayerPaths.insert(0, parent.identifier)

        with Usd.EditContext(self.stage, child):
            UsdGeom.Sphere.Define(self.stage, '/Top/FromChild').CreateRadiusAttr(9.0)
            self.stage.GetAttributeAtPath('/Top/Cube1.weakAttr').Set(9.0)
        with Usd.EditContext(self.stage, parent):
            self.stage.GetAttributeAtPath('/Top/Cube1.weakAttr').Set(7.0)

        layer, _ = self.assertSameValues(['/Top/FromChild.radius', '/Top/Cube1.weakAttr'])
        self.assertFalse(layer.GetPrimAtPath('/Top/Camera1'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
