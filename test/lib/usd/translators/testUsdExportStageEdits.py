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
Tests the export of the unsaved edits of USD stages (exportStageEdits): the session
layer, anonymous layers and the unsaved changes of modified layers are written next
to the exported file, and the exported references bring them in. Only the edits are
written: the original files are not modified nor copied (except in the documented
fallback cases).

The exported files are read in a separate process, so that nothing comes from the
layers in memory.
'''

from pxr import Sdf, Usd, UsdGeom

from maya import cmds
from maya import standalone
from maya.api import OpenMaya
import mayaUsd.lib

import filecmp
import json
import os
import shutil
import subprocess
import sys
import unittest

import fixturesUtils
import mayaUtils
import ufe
import usdUtils


# Script run in a separate process: prints the values of the queried prims and
# attributes of the exported stage, as JSON.
_QUERY_SCRIPT = '''
import json, sys
from pxr import Sdf, Usd

def toJson(value):
    if value is None:
        return None
    if isinstance(value, Sdf.AssetPath):
        return value.path
    if isinstance(value, (bool, int, float, str)):
        return value
    if isinstance(value, dict):
        return {k: toJson(v) for k, v in value.items()}
    try:
        return [toJson(v) for v in value]
    except TypeError:
        return str(value)

stage = Usd.Stage.Open(sys.argv[1])
queries = json.loads(sys.argv[2])
result = {}
for primPath, attrName, times in queries:
    prim = stage.GetPrimAtPath(primPath)
    key = primPath if not attrName else primPath + '.' + attrName
    if not attrName:
        result[key] = {'exists': bool(prim), 'active': bool(prim) and prim.IsActive(),
                       'defined': bool(prim) and prim.IsDefined()}
        continue
    if attrName.startswith('rel:'):
        rel = prim.GetRelationship(attrName[4:]) if prim else None
        result[key] = [str(t) for t in rel.GetTargets()] if rel else None
        continue
    if attrName.startswith('meta:'):
        result[key] = toJson(prim.GetMetadata(attrName[5:])) if prim else None
        continue
    attr = prim.GetAttribute(attrName) if prim else None
    values = []
    for time in times:
        timeCode = Usd.TimeCode.Default() if time is None else Usd.TimeCode(time)
        values.append(toJson(attr.Get(timeCode)) if attr else None)
    result[key] = values
print(json.dumps(result))
'''


class testUsdExportStageEdits(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

        # Each test uses its own copy of the files, so that the layers modified in
        # memory by one test are not seen by the next tests.
        self._dataDir = os.path.abspath('data_%s' % self._testMethodName)
        if os.path.exists(self._dataDir):
            shutil.rmtree(self._dataDir)
        shutil.copytree(os.path.join(self.inputPath, 'UsdExportStageEditsTest'), self._dataDir)
        self._originalDataDir = os.path.join(self.inputPath, 'UsdExportStageEditsTest')

    def _createStage(self, fileName='root.usda'):
        '''Create a proxy shape on a file of the test data.'''
        transform = cmds.createNode('transform', name='stage1')
        shape = cmds.createNode('mayaUsdProxyShape', name='stageShape1', parent=transform)
        cmds.setAttr(shape + '.filePath', self._dataFile(fileName), type='string')
        self._proxyPath = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(self._proxyPath).GetStage()
        self.assertTrue(stage)
        return stage

    def _dataFile(self, fileName):
        return os.path.join(self._dataDir, fileName)

    def _layer(self, fileName):
        layer = Sdf.Layer.Find(self._dataFile(fileName))
        self.assertTrue(layer, fileName)
        return layer

    def _export(self, fileName=None, **kwargs):
        '''
        Export the Maya scene. The warnings displayed during the export are kept
        in self._warnings.
        '''
        self._warnings = []
        def _onOutput(message, messageType, _):
            if messageType == OpenMaya.MCommandMessage.kWarning:
                self._warnings.append(message)
        callbackId = OpenMaya.MCommandMessage.addCommandOutputCallback(_onOutput, None)

        self._usdFilePath = os.path.abspath(fileName or ('%s.usda' % self._testMethodName))
        self._editsDir = os.path.join(
            os.path.dirname(self._usdFilePath),
            os.path.splitext(os.path.basename(self._usdFilePath))[0] + '_edits',
            'stageShape1')
        if os.path.exists(os.path.dirname(self._editsDir)):
            shutil.rmtree(os.path.dirname(self._editsDir))

        try:
            cmds.mayaUSDExport(
                file=self._usdFilePath,
                mergeTransformAndShape=True,
                shadingMode='none',
                defaultPrim='stage1',
                exportStagesAsRefs=True,
                **kwargs)
        finally:
            OpenMaya.MMessage.removeCallback(callbackId)

    def _query(self, queries):
        '''
        Read the values of prims and attributes of the exported file in a separate
        process. Each query is (primPath, attrName or None, times).
        '''
        process = subprocess.run(
            [sys.executable, '-c', _QUERY_SCRIPT, self._usdFilePath, json.dumps(queries)],
            capture_output=True, text=True, timeout=300)
        self.assertEqual(process.returncode, 0, process.stderr)
        return json.loads(process.stdout.strip().splitlines()[-1])

    def _assertMatchesStage(self, stage, attributes, times=(None, 0, 5, 10, 20, 30)):
        '''
        Verify that the attributes of the exported file have the same values as in the
        stage of the proxy shape. The attributes are given by their path in the stage.
        '''
        queries = [['/stage1' + str(path.GetPrimPath()), path.name, list(times)]
                   for path in map(Sdf.Path, attributes)]
        exported = self._query(queries)
        for attrPath, query in zip(attributes, queries):
            attr = stage.GetAttributeAtPath(attrPath)
            expected = []
            for time in times:
                value = attr.Get(Usd.TimeCode.Default() if time is None else Usd.TimeCode(time)) if attr else None
                if isinstance(value, Sdf.AssetPath):
                    value = value.path
                elif value is not None and not isinstance(value, (bool, int, float, str)):
                    value = list(value)
                expected.append(value)
            actual = exported[query[0] + '.' + query[1]]
            self.assertEqual(len(actual), len(expected))
            for time, a, e in zip(times, actual, expected):
                if isinstance(e, float):
                    self.assertAlmostEqual(a, e, msg='%s at %s' % (attrPath, time))
                else:
                    self.assertEqual(a, e, '%s at %s' % (attrPath, time))

    def _referencedAssetPaths(self):
        '''Returns the asset paths referenced by the exported root prims of the stage.'''
        layer = Sdf.Layer.OpenAsAnonymous(self._usdFilePath)
        prim = layer.GetPrimAtPath('/stage1/Top')
        self.assertTrue(prim, 'No /stage1/Top in the exported file')
        return [ref.assetPath for ref in prim.referenceList.prependedItems]

    def _editsFiles(self):
        if not os.path.isdir(self._editsDir):
            return []
        return sorted(os.listdir(self._editsDir))

    def _assertSamePath(self, a, b):
        self.assertEqual(os.path.normcase(os.path.normpath(a)), os.path.normcase(os.path.normpath(b)))

    def _assertOriginalFilesUnchanged(self):
        '''The edits must never be saved into the original files.'''
        for fileName in ['root.usda', 'sub.usda', 'anim.usda', 'anim48.usda', 'asset.usda',
                         'asset48.usda']:
            self.assertTrue(filecmp.cmp(self._dataFile(fileName),
                                        os.path.join(self._originalDataDir, fileName),
                                        shallow=False), fileName)

    def testCleanStage(self):
        '''A stage without unsaved edits is exported as before, without extra files.'''
        self._createStage()
        self._export()

        self.assertFalse(os.path.exists(os.path.dirname(self._editsDir)))
        paths = self._referencedAssetPaths()
        self.assertEqual(len(paths), 1)
        self._assertSamePath(paths[0], self._dataFile('root.usda'))

    def testEdits(self):
        '''
        Edits in the session layer, the root layer and sublayers (with offsets and other
        TCPS) are exported in a single edits layer, above the original files, and the
        exported stage has the same values as the stage.
        '''
        stage = self._createStage()

        with Usd.EditContext(stage, stage.GetRootLayer()):
            UsdGeom.XformCommonAPI(stage.GetPrimAtPath('/Top/Cube1')).SetTranslate((5, 6, 7))
            stage.GetAttributeAtPath('/Top/Cylinder1.radius').Set(3.0)
            UsdGeom.Sphere.Define(stage, '/Top/NewSphere').CreateRadiusAttr(2.0)
            stage.RemovePrim('/Top/Camera1')
            # Relative asset paths, with and without "./".
            stage.GetAttributeAtPath('/Top/Cube1.texFile').Set(Sdf.AssetPath('./textures/other.png'))
            stage.GetAttributeAtPath('/Top/Cube1.searchTex').Set(Sdf.AssetPath('textures/other.png'))
        # Removed values: the weaker value (3), or no value.
        rootLayer = stage.GetRootLayer()
        rootLayer.GetAttributeAtPath('/Top/Cube1.removable').ClearDefaultValue()
        rootLayer.GetAttributeAtPath('/Top/Cube1.blockable').ClearDefaultValue()

        with Usd.EditContext(stage, self._layer('sub.usda')):
            cube = stage.GetPrimAtPath('/Top/Cube1')
            cube.GetAttribute('sharedAttr').Set(20.0)  # hidden by the root layer (10)
            cube.GetAttribute('weakAttr').Set(30.0)
            # Hidden by the explicit list of the root layer.
            cube.GetRelationship('explicitTargets').SetTargets([Sdf.Path('/Top/Cone1')])
            # The root layer prepends this target again.
            cube.GetRelationship('prependedTargets').RemoveTarget(Sdf.Path('/Top/Cylinder1'))
            # The root layer has an over.
            stage.DefinePrim('/Top/DefinedInSub', 'Xform')
            # The root layer selects another variant set, and has another key.
            stage.GetPrimAtPath('/Top/Cone1').GetVariantSet('color').SetVariantSelection('blue')
            stage.GetPrimAtPath('/Top/Cylinder1').SetCustomDataByKey('nested:y', 2)

        # A sublayer with an offset and a scale.
        heightPath = Sdf.Path('/Top/Cylinder1.height')
        for time, value in [(10.0, 5.0), (20.0, 9.0)]:
            self._layer('anim.usda').SetTimeSample(heightPath, time, value)

        # A reference in a sublayer with another TCPS than the stage.
        with Usd.EditContext(stage, self._layer('anim48.usda')):
            stage.OverridePrim('/Top/Anim48Ref').GetReferences().AddReference('./asset48.usda')

        with Usd.EditContext(stage, stage.GetSessionLayer()):
            UsdGeom.XformCommonAPI(stage.GetPrimAtPath('/Top/Cone1')).SetTranslate((0, 2, 0))
            stage.GetPrimAtPath('/Top/Cone1').GetVariantSet('size').SetVariantSelection('big')
            UsdGeom.Xform.Define(stage, '/SessionRoot')

        self._export()

        self.assertEqual(self._editsFiles(), ['edits.usdc', 'stage.usda'])
        self.assertEqual(self._referencedAssetPaths(),
                         ['./%s_edits/stageShape1/stage.usda' % self._testMethodName])
        self._assertOriginalFilesUnchanged()

        # The wrapper has the edits above the root layer on disk.
        wrapper = Sdf.Layer.OpenAsAnonymous(os.path.join(self._editsDir, 'stage.usda'))
        self.assertEqual(list(wrapper.subLayerPaths)[0], './edits.usdc')
        self._assertSamePath(list(wrapper.subLayerPaths)[1], self._dataFile('root.usda'))
        self.assertEqual(wrapper.defaultPrim, 'Top')

        # Only the edited fields are in the edits, not a copy of the layers.
        edits = Sdf.Layer.OpenAsAnonymous(os.path.join(self._editsDir, 'edits.usdc'))
        self.assertFalse(edits.GetPrimAtPath('/Top/Ref'))
        self.assertFalse(edits.GetAttributeAtPath('/Top/Cube1.xformOpOrder'))
        self.assertFalse(edits.GetPrimAtPath('/Top/Cone1{size=big}'))

        self._assertMatchesStage(stage, [
            '/Top/Cube1.xformOp:translate',
            '/Top/Cube1.sharedAttr',
            '/Top/Cube1.weakAttr',
            '/Top/Cube1.removable',
            '/Top/Cube1.blockable',
            '/Top/Cylinder1.radius',
            '/Top/Cylinder1.height',
            '/Top/NewSphere.radius',
            '/Top/Cone1.xformOp:translate',
            '/Top/Cone1.radius',
            '/Top/Cone1.colorValue',
            '/Top/Anim48Ref.value',
        ], times=(None, 0, 5, 10, 12, 20, 24, 30, 48))

        result = self._query([
            ['/stage1/Top/Camera1', None, []],
            ['/stage1/Top/DefinedInSub', None, []],
            ['/stage1/SessionRoot', None, []],
            ['/stage1/Top/Cube1', 'rel:explicitTargets', []],
            ['/stage1/Top/Cube1', 'rel:prependedTargets', []],
            ['/stage1/Top/Cylinder1', 'meta:customData', []],
            ['/stage1/Top/Cube1', 'texFile', [None]],
            ['/stage1/Top/Cube1', 'searchTex', [None]],
        ])
        self.assertFalse(result['/stage1/Top/Camera1']['active'])
        self.assertTrue(result['/stage1/Top/DefinedInSub']['defined'])
        self.assertTrue(result['/stage1/SessionRoot']['exists'])
        self.assertEqual(result['/stage1/Top/Cube1.rel:explicitTargets'], ['/stage1/Top/Cylinder1'])
        self.assertEqual(result['/stage1/Top/Cube1.rel:prependedTargets'], ['/stage1/Top/Cylinder1'])
        self.assertEqual(result['/stage1/Top/Cylinder1.meta:customData']['nested'], {'x': 1, 'y': 2})
        for attr in ['texFile', 'searchTex']:
            self._assertSamePath(result['/stage1/Top/Cube1.' + attr][0],
                                 self._dataFile('textures/other.png'))

    def testRenamedPrim(self):
        '''A renamed prim: the old prim is deactivated, the new one is exported.'''
        stage = self._createStage()
        with Usd.EditContext(stage, stage.GetRootLayer()):
            Sdf.CopySpec(stage.GetRootLayer(), '/Top/Cylinder1', stage.GetRootLayer(), '/Top/Renamed')
            stage.RemovePrim('/Top/Cylinder1')

        self._export()

        result = self._query([['/stage1/Top/Cylinder1', None, []], ['/stage1/Top/Renamed', None, []]])
        # The sublayer still has an over on the old prim, which is not defined anymore.
        self.assertFalse(stage.GetPrimAtPath('/Top/Cylinder1').IsDefined())
        self.assertFalse(result['/stage1/Top/Cylinder1']['active'])
        self.assertTrue(result['/stage1/Top/Renamed']['active'])
        self._assertMatchesStage(stage, ['/Top/Renamed.radius', '/Top/Renamed.height'])

    def testAnonymousRootStage(self):
        '''A stage with an anonymous root layer is now exported.'''
        shapeNode, stage = mayaUtils.createProxyAndStage()
        UsdGeom.Sphere.Define(stage, '/Ball').CreateRadiusAttr(4.0)
        stage.SetDefaultPrim(stage.GetPrimAtPath('/Ball'))
        self._proxyPath = shapeNode

        self._usdFilePath = os.path.abspath('%s.usda' % self._testMethodName)
        cmds.mayaUSDExport(file=self._usdFilePath, mergeTransformAndShape=True,
                           shadingMode='none', exportStagesAsRefs=True)

        exportedStage = Usd.Stage.Open(self._usdFilePath)
        balls = [prim for prim in exportedStage.Traverse() if prim.GetName() == 'Ball']
        self.assertEqual(len(balls), 1)
        self.assertEqual(balls[0].GetAttribute('radius').Get(), 4.0)

    def testUnsharedStage(self):
        '''The anonymous root layer of an unshared stage is exported.'''
        self._createStage()
        cmds.setAttr(self._proxyPath + '.shareStage', False)
        stage = mayaUsd.lib.GetPrim(self._proxyPath).GetStage()
        self.assertTrue(stage.GetRootLayer().anonymous)
        with Usd.EditContext(stage, stage.GetRootLayer()):
            UsdGeom.Xform.Define(stage, '/Top/AddedInUnsharedRoot')

        self._export()

        result = self._query([['/stage1/Top/AddedInUnsharedRoot', None, []],
                              ['/stage1/Top/Cube1', None, []]])
        self.assertTrue(result['/stage1/Top/AddedInUnsharedRoot']['exists'])
        self.assertTrue(result['/stage1/Top/Cube1']['exists'])
        # The Maya-USD bookkeeping of unshared stages is not exported.
        wrapper = Sdf.Layer.OpenAsAnonymous(os.path.join(self._editsDir, 'stage.usda'))
        self.assertNotIn('mayaSharedLayers', wrapper.customLayerData)

    def testMutedSubLayer(self):
        '''
        The opinions of a muted sublayer are removed by the edits. Its parent layer is
        not copied.
        '''
        stage = self._createStage()
        stage.MuteLayer(self._layer('sub.usda').identifier)
        self.assertFalse(stage.GetAttributeAtPath('/Top/Cube1.weakAttr'))

        self._export()

        self.assertEqual(self._editsFiles(), ['edits.usdc', 'stage.usda'])
        edits = Sdf.Layer.OpenAsAnonymous(os.path.join(self._editsDir, 'edits.usdc'))
        self.assertFalse(edits.GetPrimAtPath('/Top/Camera1'))
        self._assertMatchesStage(stage, [
            '/Top/Cube1.weakAttr',
            '/Top/Cube1.removable',
            '/Top/Cylinder1.height',
        ])
        self._assertOriginalFilesUnchanged()

    def testDirtyReferencedLayer(self):
        '''
        The unsaved changes of a layer reached through a reference are not exported,
        with a warning.
        '''
        stage = self._createStage()
        assetLayer = self._layer('asset.usda')
        assetLayer.GetAttributeAtPath('/Ball.radius').default = 5.0
        del assetLayer.GetPrimAtPath('/Ball').nameChildren['Extra']
        self.assertEqual(stage.GetAttributeAtPath('/Top/Ref.radius').Get(), 5.0)
        self.assertFalse(stage.GetPrimAtPath('/Top/Ref/Extra'))

        self._export()

        self.assertEqual(self._editsFiles(), [])
        self.assertTrue(any('are not exported' in w and 'asset.usda' in w
                            for w in self._warnings), self._warnings)
        result = self._query([['/stage1/Top/Ref', 'radius', [None]]])
        self.assertEqual(result['/stage1/Top/Ref.radius'], [1.0])
        self._assertOriginalFilesUnchanged()

    def testOptionOff(self):
        '''With the option off, the edits are not exported, as before.'''
        stage = self._createStage()
        with Usd.EditContext(stage, stage.GetRootLayer()):
            stage.GetPrimAtPath('/Top/Cylinder1').GetAttribute('radius').Set(3.0)

        self._export(exportStageEdits=False)

        self.assertFalse(os.path.exists(os.path.dirname(self._editsDir)))
        self.assertTrue(any("aren't saved" in w for w in self._warnings), self._warnings)
        result = self._query([['/stage1/Top/Cylinder1', 'radius', [None]]])
        self.assertEqual(result['/stage1/Top/Cylinder1.radius'], [1.0])

    def testUsdzExport(self):
        '''The edits are packaged in the usdz file, and the temporary files are deleted.'''
        stage = self._createStage()
        with Usd.EditContext(stage, stage.GetRootLayer()):
            stage.GetPrimAtPath('/Top/Cylinder1').GetAttribute('radius').Set(3.0)

        self._usdFilePath = os.path.abspath('%s.usdz' % self._testMethodName)
        cmds.mayaUSDExport(file=self._usdFilePath, mergeTransformAndShape=True,
                           shadingMode='none', defaultPrim='stage1', exportStagesAsRefs=True)

        fileNames = Sdf.ZipFile.Open(self._usdFilePath).GetFileNames()
        self.assertTrue(any(name.endswith('stage.usda') for name in fileNames), fileNames)
        self.assertTrue(any(name.endswith('edits.usdc') for name in fileNames), fileNames)

        # The edits were written next to the temporary file that was packaged.
        leftovers = [name for name in os.listdir(os.path.dirname(self._usdFilePath))
                     if name.startswith('tmp-') and name.endswith('_edits')]
        self.assertEqual(leftovers, [])

        result = self._query([['/stage1/Top/Cylinder1', 'radius', [None]]])
        self.assertEqual(result['/stage1/Top/Cylinder1.radius'], [3.0])

    def testMayaUsdBookkeepingIsNotExported(self):
        '''
        The session layer opinions that Maya-USD authors for its own needs (prims
        edited as Maya, Maya reference node names, Outliner colors) are not exported,
        and don't make a stage dirty. The prim edited as Maya is exported active.
        '''
        stage = self._createStage()
        with mayaUsd.lib.OpUndoItemList():
            self.assertTrue(mayaUsd.lib.PrimUpdaterManager.editAsMaya(
                self._proxyPath + ',/Top/Cone1'))
        self.assertFalse(stage.GetPrimAtPath('/Top/Cone1').IsActive())
        with Usd.EditContext(stage, stage.GetSessionLayer()):
            prim = stage.GetPrimAtPath('/Top/Cylinder1')
            prim.CreateAttribute('MayaReferenceNodeName', Sdf.ValueTypeNames.String).Set('refRN')
            prim.SetCustomDataByKey('Autodesk:Use Text Color', True)
            prim.SetCustomDataByKey('Autodesk:Text Color', (1.0, 0.0, 0.0))

        self._export()

        self.assertFalse(os.path.exists(os.path.dirname(self._editsDir)))
        result = self._query([['/stage1/Top/Cone1', None, []]])
        self.assertTrue(result['/stage1/Top/Cone1']['active'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
