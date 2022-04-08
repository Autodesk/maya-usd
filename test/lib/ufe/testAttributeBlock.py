#!/usr/bin/env python

#
# Copyright 2021 Autodesk
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
import ufeUtils
import usdUtils

from pxr import UsdGeom, Vt, Gf, Usd, Sdf

from maya import cmds
from maya import standalone

import mayaUsd
from mayaUsd import ufe as mayaUsdUfe

import ufe
import os
import unittest
import textwrap


class AttributeBlockTestCase(unittest.TestCase):

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
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)

        # Clear selection to start off
        cmds.select(clear=True)

    def testSingleAttributeBlocking(self):
        ''' Authoring attribute(s) in weaker layer(s) are not permitted if there exist opinion(s) in stronger layer(s).'''

        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # cylinder prim
        cylinderPrim = stage.GetPrimAtPath('/pCylinder1')
        self.assertIsNotNone(cylinderPrim)

        # author a new radius value
        self.assertTrue(cylinderPrim.HasAttribute('doubleSided'))
        doubleSidedAttr = cylinderPrim.GetAttribute('doubleSided')

        # authoring new attribute edit is expected to be allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(doubleSidedAttr))
        doubleSidedAttr.Set(0)

        # create a sub-layer.
        rootLayer = stage.GetRootLayer()
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="LayerA")[0]

        # check to see the if the sublayers was added
        addedLayers = [subLayerA]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerA.
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # doubleSidedAttr is not allowed to change since there is an opinion in a stronger layer
        self.assertFalse(mayaUsdUfe.isAttributeEditAllowed(doubleSidedAttr))

    def testTransformationAttributeBlocking(self):
        '''Authoring transformation attribute(s) in weaker layer(s) are not permitted if there exist opinion(s) in stronger layer(s).'''

        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # cylinder prim
        cylinderPrim = stage.GetPrimAtPath('/pCylinder1')
        self.assertIsNotNone(cylinderPrim)

         # create 3 sub-layers ( LayerA, LayerB, LayerC ) and set the edit target to LayerB.
        rootLayer = stage.GetRootLayer()
        subLayerC = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerC")[0]
        subLayerB = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerB")[0]
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerA")[0]

        # check to see the sublayers added
        addedLayers = [subLayerA, subLayerB, subLayerC]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerB
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerB)

        # create a transform3d
        cylinderPath = ufe.Path([
                     mayaUtils.createUfePathSegment("|mayaUsdTransform|shape"), 
                     usdUtils.createUfePathSegment("/pCylinder1")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        cylinderT3d = ufe.Transform3d.transform3d(cylinderItem)

        # create a xformable
        cylinderXformable = UsdGeom.Xformable(cylinderPrim)

        # writing to "transform op order" is expected to be allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(cylinderXformable.GetXformOpOrderAttr()))

        # do any transform editing.
        cylinderT3d = ufe.Transform3d.transform3d(cylinderItem)
        cylinderT3d.scale(2.5, 2.5, 2.5)
        cylinderT3d.translate(4.0, 2.0, 3.0)

        # check the "transform op order" stack.
        self.assertEqual(cylinderXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray(('xformOp:translate', 'xformOp:scale')))

        # check if translate attribute is editable
        translateAttr = cylinderPrim.GetAttribute('xformOp:translate')
        self.assertIsNotNone(translateAttr)

        # authoring new transformation edit is expected to be allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(translateAttr))
        cylinderT3d.translate(5.0, 6.0, 7.0)
        self.assertEqual(translateAttr.Get(), Gf.Vec3d(5.0, 6.0, 7.0))

        # set the edit target to a weaker layer (LayerC)
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerC)

        # authoring new transformation edit is not allowed.
        self.assertFalse(mayaUsdUfe.isAttributeEditAllowed(translateAttr))

        # set the edit target to a stronger layer (LayerA)
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # authoring new transformation edit is allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(translateAttr))
        cylinderT3d.rotate(0.0, 90.0, 0.0)

        # check the "transform op order" stack.
        self.assertEqual(cylinderXformable.GetXformOpOrderAttr().Get(), Vt.TokenArray(('xformOp:translate','xformOp:rotateXYZ', 'xformOp:scale')))

    def testAttributeBlocking3dCommonApi(self):
        '''
        Verify authoring transformation attribute(s) in weaker layer(s) are not permitted if there exist opinion(s) 
        in stronger layer(s) when using UsdGeomXformCommonAPI.
        '''

        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # cylinder prim
        cylinderPrim = stage.GetPrimAtPath('/pCylinder1')
        self.assertIsNotNone(cylinderPrim)

         # create 3 sub-layers ( LayerA, LayerB, LayerC ) and set the edit target to LayerB.
        rootLayer = stage.GetRootLayer()
        subLayerC = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerC")[0]
        subLayerB = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerB")[0]
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerA")[0]

        # check to see the sublayers added
        addedLayers = [subLayerA, subLayerB, subLayerC]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerB
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerB)

        # create a xformable and give it a common API single "pivot".
        # This will match the common API, but not the Maya API.
        cylinderXformable = UsdGeom.Xformable(cylinderPrim)

        cylinderXformable.AddTranslateOp(UsdGeom.XformOp.PrecisionFloat, "pivot")
        cylinderXformable.AddTranslateOp(
            UsdGeom.XformOp.PrecisionFloat, "pivot", True)

        self.assertEqual(
            cylinderXformable.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(("xformOp:translate:pivot",
                           "!invert!xformOp:translate:pivot")))
        self.assertTrue(UsdGeom.XformCommonAPI(cylinderXformable))

        # Now that we have set up the transform stack to be common API-compatible,
        # create the Transform3d interface.
        cylinderPath = ufe.Path([
                     mayaUtils.createUfePathSegment("|mayaUsdTransform|shape"), 
                     usdUtils.createUfePathSegment("/pCylinder1")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        cylinderT3d = ufe.Transform3d.transform3d(cylinderItem)

        # select the cylinderItem
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(cylinderItem)

        # rotate the cylinder
        cmds.rotate(0, 90, 0, relative=True, objectSpace=True, forceOrderXYZ=True)

        self.assertEqual(
            cylinderXformable.GetXformOpOrderAttr().Get(), 
            Vt.TokenArray(("xformOp:translate:pivot", "xformOp:rotateXYZ",
                           "!invert!xformOp:translate:pivot")))
        self.assertTrue(UsdGeom.XformCommonAPI(cylinderXformable))

        # set the edit target to a weaker layer (LayerC)
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerC)

        # check if rotate attribute is editable
        rotateAttr = cylinderPrim.GetAttribute('xformOp:rotateXYZ')
        self.assertIsNotNone(rotateAttr)

        # authoring new transformation edit is not allowed.
        self.assertFalse(mayaUsdUfe.isAttributeEditAllowed(rotateAttr))

        # set the edit target to a stronger layer (LayerA)
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # authoring new transformation edit is allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(rotateAttr))

    @unittest.skipUnless(ufeUtils.ufeFeatureSetVersion() >= 2, 'testAttributeBlocking3dMatrixOps only available in UFE v2 or greater.')
    def testAttributeBlocking3dMatrixOps(self):
        '''
        Verify authoring transformation attribute(s) in weaker layer(s) are not permitted if there exist opinion(s) 
        in stronger layer(s) when using UsdTransform3dMatrixOp.
        '''

        # create new stage
        cmds.file(new=True, force=True)

        # Open usdCylinder.ma scene in testSamples
        mayaUtils.openCylinderScene()

        # get the stage
        proxyShapes = cmds.ls(type="mayaUsdProxyShapeBase", long=True)
        proxyShapePath = proxyShapes[0]
        stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()

        # cylinder prim
        cylinderPrim = stage.GetPrimAtPath('/pCylinder1')
        self.assertIsNotNone(cylinderPrim)

         # create 3 sub-layers ( LayerA, LayerB, LayerC ) and set the edit target to LayerB.
        rootLayer = stage.GetRootLayer()
        subLayerC = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerC")[0]
        subLayerB = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerB")[0]
        subLayerA = cmds.mayaUsdLayerEditor(rootLayer.identifier, edit=True, addAnonymous="SubLayerA")[0]

        # check to see the sublayers added
        addedLayers = [subLayerA, subLayerB, subLayerC]
        self.assertEqual(rootLayer.subLayerPaths, addedLayers)

        # set the edit target to LayerB
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerB)

        # create a xformable and give it a matrix4d xformOp:transform
        # This will match the UsdTransform3dMatrixOp, but not the Maya API.
        cylinderXformable = UsdGeom.Xformable(cylinderPrim)
        transformOp = cylinderXformable.AddTransformOp()
        xform = Gf.Matrix4d(1.0).SetTranslate(Gf.Vec3d(10, 20, 30))
        transformOp.Set(xform)
        self.assertTrue(cylinderPrim.GetPrim().HasAttribute("xformOp:transform"))

        # Now that we have set up the transform stack to be 3dMatrixOp compatible,
        # create the Transform3d interface.
        cylinderPath = ufe.Path([
                     mayaUtils.createUfePathSegment("|mayaUsdTransform|shape"), 
                     usdUtils.createUfePathSegment("/pCylinder1")])
        cylinderItem = ufe.Hierarchy.createItem(cylinderPath)

        cylinderT3d = ufe.Transform3d.transform3d(cylinderItem)

        # select the cylinderItem
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(cylinderItem)

        # move
        cmds.move(65, 20, 10)
        self.assertEqual(cylinderXformable.GetXformOpOrderAttr().Get(),
                    Vt.TokenArray(('xformOp:transform', )))

        # validate the Matrix4d entires
        actual = cylinderXformable.GetLocalTransformation()
        expected  = Gf.Matrix4d( 1, 0, 0, 0,
                                 0, 1, 0, 0,
                                 0, 0, 1, 0,
                                 65, 20, 10, 1)

        self.assertTrue(Gf.IsClose(actual, expected, 1e-5))

        # set the edit target to a weaker layer (LayerC)
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerC)

        # check if transform attribute is editable
        transformAttr = cylinderPrim.GetAttribute('xformOp:transform')
        self.assertIsNotNone(transformAttr)

        # authoring new transformation edit is not allowed.
        self.assertFalse(mayaUsdUfe.isAttributeEditAllowed(transformAttr))

        # set the edit target to a stronger layer (LayerA)
        cmds.mayaUsdEditTarget(proxyShapePath, edit=True, editTarget=subLayerA)

        # authoring new transformation edit is allowed.
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(transformAttr))
    
    def testIsAttributeEditAllowedFunction(self):
        rootLayerStr = """\
        #sdf 1.4.32
        ()
        def "root"
        {
        }
        """
        subLayerAStr = """\
        #sdf 1.4.32
        over "root"
        {
            def "a" 
            {
                float foo = 0.0
            }

            def "b" 
            {
                float foo = 0.0
            }
        }
        """
        subLayerBStr = """\
        #sdf 1.4.32
        over "root"
        {
        }
        """
        subLayerCStr = """\
        #sdf 1.4.32
        over "root"
        {
            over "a" 
            {
                float foo = 10.0
            }
        }
        """

        rootLayer = Sdf.Layer.CreateAnonymous()
        rootLayer.ImportFromString(textwrap.dedent(rootLayerStr))

        subLayerA = Sdf.Layer.CreateAnonymous()
        subLayerA.ImportFromString(textwrap.dedent(subLayerAStr))

        subLayerB = Sdf.Layer.CreateAnonymous()
        subLayerB.ImportFromString(textwrap.dedent(subLayerBStr))

        subLayerC = Sdf.Layer.CreateAnonymous()
        subLayerC.ImportFromString(textwrap.dedent(subLayerCStr))

        rootLayer.subLayerPaths = [
            subLayerC.identifier,
            subLayerB.identifier,
            subLayerA.identifier,
        ]
        stage = Usd.Stage.Open(rootLayer)

        primB = stage.GetPrimAtPath("/root/b")
        self.assertTrue(primB.IsValid())

        fooattrB = primB.GetAttribute("foo")
        self.assertTrue(fooattrB.IsValid())

        stage.SetEditTarget(Usd.EditTarget(subLayerB))
        self.assertTrue(mayaUsdUfe.isAttributeEditAllowed(fooattrB))

        primA = stage.GetPrimAtPath("/root/a")
        self.assertTrue(primA.IsValid())

        fooattrA = primA.GetAttribute("foo")
        self.assertTrue(fooattrA.IsValid())

        self.assertFalse(mayaUsdUfe.isAttributeEditAllowed(fooattrA))


if __name__ == '__main__':
    unittest.main(verbosity=2)
