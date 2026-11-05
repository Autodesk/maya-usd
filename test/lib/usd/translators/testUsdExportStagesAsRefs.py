#!/usr/bin/env mayapy
#
# Copyright 2024 Autodesk
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

from pxr import Sdf, Usd, UsdShade

from maya import cmds
from maya import standalone
from maya.api import OpenMaya
import mayaUsd.lib

import os
import unittest

import fixturesUtils
import mayaUtils


class testUsdExportStagesAsRefs(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _prepareScene(self):
        mayaFileName  = 'sceneWithStage.ma'
        self._mayaFile = os.path.join(self.inputPath, 'UsdExportStagesAsRefsTest', mayaFileName)
        cmds.file(self._mayaFile, force=True, open=True)

    def _prepareMultiRootScene(self, usdFileName='multiRootStage.usda'):
        '''Point the scene stage to a USD file containing multiple root prims.'''
        self._prepareScene()
        usdFile = os.path.join(self.inputPath, 'UsdExportStagesAsRefsTest', usdFileName)
        cmds.setAttr('|stage1|stageShape1.filePath', usdFile, type='string')

    def _removeCube1FromSceneStage(self):
        '''Modify the stage contained in the Maya scene to make it dirty.'''
        proxyShapePath = '|stage1|stageShape1'
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
        self.assertTrue(stage)
        primPath = '/Top/Cube1'
        self.assertTrue(stage.GetPrimAtPath(primPath))
        stage.RemovePrim(primPath)
        self.assertFalse(stage.GetPrimAtPath(primPath))

    def _export(self, exportStagesAsRef=True, selection=None, mergeTransform=True, **kwargs):
        '''
        Export the Maya scene while converting USD stages to USD references.
        Then open the exported USD file. The warnings displayed during the export
        are kept in self._warnings.
        '''
        if selection:
            cmds.select(selection)

        self._warnings = []
        def _onOutput(message, messageType, _):
            if messageType == OpenMaya.MCommandMessage.kWarning:
                self._warnings.append(message)
        callbackId = OpenMaya.MCommandMessage.addCommandOutputCallback(_onOutput, None)

        # Export to USD.
        self._usdFilePath = os.path.abspath('ExportStagesAsRefsTest.usda')
        try:
            cmds.mayaUSDExport(
                file=self._usdFilePath,
                mergeTransformAndShape=mergeTransform,
                shadingMode='useRegistry',
                defaultPrim='stage1',
                selection=bool(selection),
                exportStagesAsRefs=exportStagesAsRef,
                **kwargs)
        finally:
            OpenMaya.MMessage.removeCallback(callbackId)

        self._stage = Usd.Stage.Open(self._usdFilePath)

    def _verifyReference(self, expectedPath, expectPresent=True):
        '''
        Verify that the given USD reference is present or not.
        '''
        expectation = 'Expected to find' if expectPresent else 'Unexpected'
        prim = self._stage.GetPrimAtPath(expectedPath)
        self.assertEqual(bool(prim), expectPresent,
                            '%s %s when exporting %s to %s (warnings: %s)' % (
                                expectation, expectedPath, self._mayaFile, self._usdFilePath,
                                self._warnings))
        if not expectPresent:
            return
        self.assertTrue(prim.HasAuthoredReferences())
        cylPrim = self._stage.GetPrimAtPath(expectedPath + '/Cylinder1')
        self.assertTrue(cylPrim)

    def testStageOpens(self):
        '''
        Tests that the export can be done and that the USD stage can be opened successfully.
        '''
        self._prepareScene()
        self._export()
        self.assertTrue(self._stage)

    def testExportStage(self):
        '''
        Test that we can export stages and find it as a USD reference.
        '''
        self._prepareScene()
        self._export(exportStagesAsRef=True)
        self._verifyReference('/stage1/Top')
        self._verifyRootPrimReference('/stage1/Top', '/Top')

    def testExportStageDontMergeShape(self):
        '''
        Test that we can export stages without merging transform and shape and find it as a USD reference.
        '''
        self._prepareScene()
        self._export(exportStagesAsRef=True, mergeTransform=False)
        self._verifyReference('/stage1/stageShape1/Top')

    def testExportAnonymousStage(self):
        '''
        Test that anonymous stages don't get exported.
        '''
        self._prepareScene()
        
        # Create the anonymous stage. (A stage with an anonymous root layer.)
        shapeNode, shapeStage = mayaUtils.createProxyAndStage()

        self._export(exportStagesAsRef=True)
        self._verifyReference('/stage1/Top')
        self._verifyReference('/stage', expectPresent=False)

    def testDontExportStage(self):
        '''
        Test that we can turn off export stages and not find it as a USD reference.
        '''
        self._prepareScene()
        self._export(exportStagesAsRef=False)
        self._verifyReference('/stage1', expectPresent=False)

    def testExportNodeUnderStage(self):
        '''
        Test that we can turn off export stages and not find it as a USD reference
        but any Maya node that was added under the stage are kept.
        '''
        self._prepareScene()

        # Add a Maya node under the stage.
        cmds.sphere(name='pSphere1', polygon=1, radius=10)
        cmds.parent('pSphere1', 'stage1')

        self._export(exportStagesAsRef=False)
        self._verifyReference('/stage1/Top', expectPresent=False)
        self.assertTrue(self._stage.GetPrimAtPath('/stage1/pSphere1'))

    def testExportDirtyStage(self):
        '''
        Test that we can export a dirty stage and find it as a USD reference.
        '''
        self._prepareScene()
        self._removeCube1FromSceneStage()
        self._export(exportStagesAsRef=True)
        self._verifyReference('/stage1/Top')
        cubePrim = self._stage.GetPrimAtPath('/stage1/Top/Cube1')
        self.assertFalse(cubePrim)
        # The file on disk still has the cube, which the user is warned about.
        self.assertTrue(any("aren't saved" in w for w in self._warnings), self._warnings)

    def testExportMaterialsScopeClash(self):
        '''
        Test that a warning is displayed when a root prim of the stage has the same
        name as the scope of the Maya materials exported under the same prim.
        '''
        self._prepareMultiRootScene('geomLooksStage.usda')

        cube = cmds.polyCube(name='mayaCube')[0]
        shader = cmds.shadingNode('lambert', asShader=True, name='mayaLambert')
        shadingGroup = cmds.sets(renderable=True, noSurfaceShader=True, empty=True,
                                 name='mayaLambertSG')
        cmds.connectAttr(shader + '.outColor', shadingGroup + '.surfaceShader')
        cmds.sets(cube, edit=True, forceElement=shadingGroup)

        # With the default prim set to the proxy shape, the Maya materials go to
        # /stage1/Looks, where the root prim of the same name is referenced.
        # (Looks is also the scope name forced by USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME.)
        self._export(exportStagesAsRef=True, convertMaterialsTo=['UsdPreviewSurface'],
                     exportMaterials=True, materialsScopeName='Looks')
        self.assertTrue(self._stage.GetPrimAtPath('/stage1/Looks/mayaLambertSG'))
        self.assertTrue(self._stage.GetPrimAtPath('/stage1/Looks/Red'))
        clashes = [w for w in self._warnings if 'same names as prims exported from Maya' in w]
        self.assertEqual(len(clashes), 1, self._warnings)
        self.assertIn('materials scope: Looks.', clashes[0])

        # The binding between the two root prims is still fixed.
        prim = self._stage.GetPrimAtPath('/stage1/Geom/Box')
        binding = UsdShade.MaterialBindingAPI(prim).GetDirectBindingRel()
        self.assertEqual(binding.GetTargets(), [Sdf.Path('/stage1/Looks/Red')])

    def testExportSelectedStage(self):
        '''
        Test that we can export the selected stages and find it as a USD reference.
        '''
        self._prepareScene()
        self._export(exportStagesAsRef=True, selection=['stage1'])
        self._verifyReference('/stage1/Top')

    def _verifyRootPrimReference(self, exportedPath, referencedPrimPath):
        prim = self._stage.GetPrimAtPath(exportedPath)
        self.assertTrue(prim, 'Expected to find %s in %s' % (exportedPath, self._usdFilePath))
        references = prim.GetMetadata('references').GetAddedOrExplicitItems()
        self.assertEqual([ref.primPath for ref in references], [Sdf.Path(referencedPrimPath)])

    def testExportMultipleRootPrims(self):
        '''
        Test that each root prim of the stage is exported as its own USD reference,
        even when the stage has a default prim.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)
        self._verifyRootPrimReference('/stage1/Geo', '/Geo')
        self._verifyRootPrimReference('/stage1/Materials', '/Materials')
        self.assertTrue(self._stage.GetPrimAtPath('/stage1/Geo/CrossRootCube'))
        self.assertTrue(self._stage.GetPrimAtPath('/stage1/Materials/SharedMaterial'))
        # Nothing is unsaved, clashing or unloaded in this scene.
        for unexpected in ["aren't saved", 'same names as prims exported', 'unloaded payloads',
                           "couldn't be written"]:
            self.assertFalse(any(unexpected in w for w in self._warnings), self._warnings)

    def testExportMultipleRootPrimsDontMergeShape(self):
        '''
        Test that multiple root prims are exported under the shape prim when
        transform and shape are not merged.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True, mergeTransform=False)
        self._verifyRootPrimReference('/stage1/stageShape1/Geo', '/Geo')
        self._verifyRootPrimReference('/stage1/stageShape1/Materials', '/Materials')

    def testExportCrossRootRelationships(self):
        '''
        Test that relationships targeting a prim under another root prim still
        resolve after export, and that relationships within a single root prim
        are left to the reference.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)

        crossRootPrim = self._stage.GetPrimAtPath('/stage1/Geo/CrossRootCube')
        crossRootBinding = UsdShade.MaterialBindingAPI(crossRootPrim).GetDirectBindingRel()
        self.assertEqual(crossRootBinding.GetTargets(),
                         [Sdf.Path('/stage1/Materials/SharedMaterial')])

        localPrim = self._stage.GetPrimAtPath('/stage1/Geo/LocalCube')
        localBinding = UsdShade.MaterialBindingAPI(localPrim).GetDirectBindingRel()
        self.assertEqual(localBinding.GetTargets(),
                         [Sdf.Path('/stage1/Geo/LocalMaterial')])
        rootLayer = self._stage.GetRootLayer()
        self.assertFalse(rootLayer.GetRelationshipAtPath(
            '/stage1/Geo/LocalCube.material:binding'))

    def testExportCrossRootInternalReference(self):
        '''
        Test that internal references targeting a prim under another root prim
        resolve within the referenced layer, without any override on the exported stage.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)

        self.assertTrue(self._stage.GetPrimAtPath('/stage1/Geo/ProtoInstance/ProtoCube'))
        rootLayer = self._stage.GetRootLayer()
        self.assertFalse(rootLayer.GetPrimAtPath('/stage1/Geo/ProtoInstance'))

    def testExportMultipleRootPrimsSkipsClasses(self):
        '''
        Test that class root prims are not exported as references, while
        prims inheriting from them still get their content.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)

        referencedRoots = sorted(
            prim.GetName() for prim in self._stage.GetPrimAtPath('/stage1').GetAllChildren()
            if prim.HasAuthoredReferences())
        self.assertEqual(referencedRoots,
                         ['Geo', 'Materials', 'NodeGraphs', 'Prototypes'])
        self.assertFalse(self._stage.GetPrimAtPath('/stage1/_class_Instance'))
        self.assertFalse(self._stage.GetPrimAtPath('/stage1/_class_Outer'))
        # The inherits to the classes still bring in their content.
        self.assertTrue(self._stage.GetPrimAtPath('/stage1/Geo/InstanceA/InstancedCube'))

    def testExportCrossRootPurposeBinding(self):
        '''
        Test that a relationship with targets both inside and outside its root prim
        is overridden with the full list of remapped targets.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)

        prim = self._stage.GetPrimAtPath('/stage1/Geo/CrossRootCube')
        previewBinding = prim.GetRelationship('material:binding:preview')
        self.assertEqual(previewBinding.GetTargets(),
                         [Sdf.Path('/stage1/Geo/LocalMaterial'),
                          Sdf.Path('/stage1/Materials/SharedMaterial')])

        material, _ = UsdShade.MaterialBindingAPI(prim).ComputeBoundMaterial()
        self.assertEqual(material.GetPath(), Sdf.Path('/stage1/Materials/SharedMaterial'))

    def testExportCrossRootConnection(self):
        '''
        Test that attribute connections to another root prim still resolve after export,
        and that connections within a single root prim are left to the reference.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)

        sharedMaterial = self._stage.GetPrimAtPath('/stage1/Materials/SharedMaterial')
        self.assertEqual(sharedMaterial.GetAttribute('outputs:surface').GetConnections(),
                         [Sdf.Path('/stage1/NodeGraphs/SharedGraph.outputs:surface')])

        localMaterial = self._stage.GetPrimAtPath('/stage1/Geo/LocalMaterial')
        self.assertEqual(localMaterial.GetAttribute('outputs:surface').GetConnections(),
                         [Sdf.Path('/stage1/Geo/LocalMaterial/Surface.outputs:surface')])
        rootLayer = self._stage.GetRootLayer()
        self.assertFalse(rootLayer.GetAttributeAtPath(
            '/stage1/Geo/LocalMaterial.outputs:surface'))

    def testExportCrossRootTargetToClass(self):
        '''
        Test that targets pointing to a prim that is not exported (a class) are
        dropped from the override, while the other targets are kept.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)

        prim = self._stage.GetPrimAtPath('/stage1/Geo/ClassTargetCube')
        self.assertEqual(prim.GetRelationship('targets').GetTargets(),
                         [Sdf.Path('/stage1/Materials/SharedMaterial')])

    def testExportCrossRootRelationshipsInInstances(self):
        '''
        Test that instances whose content has relationships to another root prim
        are made non-instanceable so that the overrides apply.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)

        for instanceName in ['InstanceA', 'InstanceB']:
            instancePath = '/stage1/Geo/' + instanceName
            instance = self._stage.GetPrimAtPath(instancePath)
            self.assertTrue(instance)
            self.assertFalse(instance.IsInstance())

            cube = self._stage.GetPrimAtPath(instancePath + '/InstancedCube')
            self.assertFalse(cube.IsInstanceProxy())
            binding = UsdShade.MaterialBindingAPI(cube).GetDirectBindingRel()
            self.assertEqual(binding.GetTargets(),
                             [Sdf.Path('/stage1/Materials/SharedMaterial')])

    def testExportCrossRootRelationshipsInNestedInstances(self):
        '''
        Test that nested instances are all made non-instanceable, from the outermost
        one down, so that the overrides on their content apply.
        '''
        self._prepareMultiRootScene()
        self._export(exportStagesAsRef=True)

        for instancePath in ['/stage1/Geo/OuterInstance',
                             '/stage1/Geo/OuterInstance/InnerInstance']:
            instance = self._stage.GetPrimAtPath(instancePath)
            self.assertTrue(instance, instancePath)
            self.assertFalse(instance.IsInstance(), instancePath)

        cube = self._stage.GetPrimAtPath('/stage1/Geo/OuterInstance/InnerInstance/InstancedCube')
        material, _ = UsdShade.MaterialBindingAPI(cube).ComputeBoundMaterial()
        self.assertEqual(material.GetPath(), Sdf.Path('/stage1/Materials/SharedMaterial'))

    @unittest.skipIf(Usd.GetVersion() < (0, 23, 11),
                     'Path expression attributes require USD 23.11 or later.')
    def testExportCrossRootPathExpression(self):
        '''
        Test that path expressions (e.g. light linking collections) with paths under
        another root prim are remapped, and that those within a single root prim
        are left to the reference.
        '''
        self._prepareMultiRootScene('pathExpressionStage.usda')
        self._export(exportStagesAsRef=True)

        lights = self._stage.GetPrimAtPath('/stage1/Lights')
        lightLink = lights.GetAttribute('collection:lightLink:membershipExpression')
        self.assertEqual(lightLink.Get(),
                         Sdf.PathExpression('/stage1/Geo/LitCube /stage1/Lights/Key'))
        self.assertEqual(lightLink.GetVariability(), Sdf.VariabilityUniform)

        shadowLink = lights.GetAttribute('collection:shadowLink:membershipExpression')
        self.assertEqual(shadowLink.Get(), Sdf.PathExpression('/stage1/Lights/Key'))
        rootLayer = self._stage.GetRootLayer()
        self.assertFalse(rootLayer.GetAttributeAtPath(
            '/stage1/Lights.collection:shadowLink:membershipExpression'))

    def testExportIgnoresSessionLayer(self):
        '''
        Test that root prims and relationships only in the session layer of the stage
        are not exported: the references only bring in the files on disk.
        '''
        self._prepareMultiRootScene()
        stage = mayaUsd.lib.GetPrim('|stage1|stageShape1').GetStage()
        with Usd.EditContext(stage, stage.GetSessionLayer()):
            stage.DefinePrim('/SessionOnly', 'Xform')
            rel = stage.GetPrimAtPath('/Geo/LocalCube').CreateRelationship('sessionTargets')
            rel.SetTargets([Sdf.Path('/Materials/SharedMaterial')])

        self._export(exportStagesAsRef=True)

        self.assertFalse(self._stage.GetPrimAtPath('/stage1/SessionOnly'))
        rootLayer = self._stage.GetRootLayer()
        self.assertFalse(rootLayer.GetRelationshipAtPath('/stage1/Geo/LocalCube.sessionTargets'))
        # The relationships from the files are still fixed.
        prim = self._stage.GetPrimAtPath('/stage1/Geo/CrossRootCube')
        binding = UsdShade.MaterialBindingAPI(prim).GetDirectBindingRel()
        self.assertEqual(binding.GetTargets(), [Sdf.Path('/stage1/Materials/SharedMaterial')])


if __name__ == '__main__':
    unittest.main(verbosity=2)
