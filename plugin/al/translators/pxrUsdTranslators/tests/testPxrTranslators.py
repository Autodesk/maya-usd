import unittest
import tempfile
import os

import maya.cmds as mc

from pxr import Usd, Sdf

class TestTranslator(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        mc.file(f=True, new=True)
        mc.loadPlugin('pxrUsd')
        mc.loadPlugin('AL_USDMayaPlugin')

    @classmethod
    def tearDown(cls):
        mc.file(f=True, new=True)
        
    def testExportProxyShapes(self):
        import AL.usdmaya
        
        tempFile = tempfile.NamedTemporaryFile(
            suffix=".usda", prefix="AL_USDMayaTests_exportProxyShape_", delete=False)
        tempFile.close()
        
        mc.createNode("transform", n="world")
        mc.createNode("transform", n="geo", p="world")
        
        # create one proxyShape with a time offset
        mc.select(clear=1)
        proxyShapeNode = mc.AL_usdmaya_ProxyShapeImport(
            file="{}/sphere2.usda".format(os.environ.get('TEST_DIR')))[0]
        proxyShape = AL.usdmaya.ProxyShape.getByName(proxyShapeNode)
        proxyParentNode1 = mc.listRelatives(proxyShapeNode, fullPath=1, parent=1)[0]
        proxyParentNode1 = mc.parent(proxyParentNode1, "geo")[0]
        print(proxyParentNode1)
        # force the stage to load
        stage = proxyShape.getUsdStage()
        self.assertTrue(stage)

        # check that, with no time offset or scalar, we get the rotateZ values set
        # in the .usda
        sphereShape1Prim = stage.GetPrimAtPath("/pSphere1/pSphereShape1")
        sphereShape1Maya = proxyShape.makeUsdTransformChain(sphereShape1Prim)

        rotateZUsd = sphereShape1Prim.GetAttribute("xformOp:rotateZ")
        rotateZMaya = '{}.rotateZ'.format(sphereShape1Maya)
        expected_unaltered_rotate_z = {
            -1: 0,
            0: 0,
            2: 180,
            4: 360,
            5: 360,
        }
        for stage_time, expected_rotate in expected_unaltered_rotate_z.items():
            self.assertAlmostEqual(expected_rotate, rotateZUsd.Get(stage_time))
            mc.currentTime(stage_time)
            self.assertAlmostEqual(expected_rotate, mc.getAttr(rotateZMaya))

        timeOffset = 40
        timeScalarAL = 2
        mc.setAttr(proxyShapeNode + ".timeOffset", timeOffset)
        mc.setAttr(proxyShapeNode + ".timeScalar", timeScalarAL)

        # Now check that, with the time offset + scalar applied, we get
        # values at different times - note that the proxy shape interprets
        # a "timeScalar = 2" to mean the shape is fast-forwarded twice as fast,
        # so:
        #    layer_time = (maya_time - offset) * scalarAL
        #    maya_time = layer_time / scalarAL + offset
        expected_retimed_rotate_z = {
            39: 0,
            40: 0,
            41: 180,
            42: 360,
            43: 360,
        }
        for stage_time, expected_rotate in expected_retimed_rotate_z.items():
            mc.currentTime(stage_time)
            self.assertAlmostEqual(expected_rotate, mc.getAttr(rotateZMaya))

        # if we don't remove this, the reference gets made at:
        #     /world/geo/AL_usdmaya_Proxy/AL_usdmaya_ProxyShape
        # instead of the (collapsed):
        #     /world/geo/AL_usdmaya_Proxy
        proxyShape.removeUsdTransformChain(sphereShape1Prim)

        # create another proxyShape with a few session layer edits
        mc.select(clear=1)
        proxyShapeNode2 = mc.AL_usdmaya_ProxyShapeImport(
            file="{}/sphere2.usda".format(os.environ.get('TEST_DIR')))[0]
        proxyShape2 = AL.usdmaya.ProxyShape.getByName(proxyShapeNode2)
        proxyParentNode2 = mc.listRelatives(proxyShapeNode2, fullPath=1, parent=1)[0]
        proxyParentNode2 = mc.parent(proxyParentNode2, "geo")[0]
        print(proxyParentNode2)
        # force the stage to load
        stage2 = proxyShape2.getUsdStage()
        self.assertTrue(stage2)
        
        
        session = stage2.GetSessionLayer()
        stage2.SetEditTarget(session)
        extraPrimPath = "/pExtraPrimPath"
        firstSpherePath = "/pSphereShape1"
        secondSpherePath = "/pSphereShape2"
        stage2.DefinePrim("/pSphere1" + extraPrimPath)
        existingSpherePath = "/pSphere1" + secondSpherePath
        self.assertTrue(stage2.GetPrimAtPath(existingSpherePath))
        stage2.DefinePrim(existingSpherePath).SetActive(False)
        
        mc.select("world")
        mc.usdExport(f=tempFile.name)
        
        resultStage = Usd.Stage.Open(tempFile.name)
        self.assertTrue(resultStage)
        rootLayer = resultStage.GetRootLayer()
        
        refPrimPath = "/world/geo/" + proxyParentNode1
        refPrimPath2 = "/world/geo/" + proxyParentNode2
        print("Ref Prim Path 1: " + refPrimPath)
        print("Ref Prim Path 2: " + refPrimPath2)
        print("Resulting stage contents:")
        print(rootLayer.ExportToString())
        
        # Check proxyShape1
        # make sure references were created and that they have correct offset + scale
        refSpec = rootLayer.GetPrimAtPath(refPrimPath)
        self.assertTrue(refSpec)
        self.assertTrue(refSpec.hasReferences)
        refs = refSpec.referenceList.GetAddedOrExplicitItems()
        self.assertEqual(refs[0].layerOffset, Sdf.LayerOffset(timeOffset, 1.0/timeScalarAL))

        # and check that the animated values are properly offset
        resultSphereShape1Prim = resultStage.GetPrimAtPath(refPrimPath + firstSpherePath)
        rotateZUsd = resultSphereShape1Prim.GetAttribute("xformOp:rotateZ")
        for stage_time, expected_rotate in expected_retimed_rotate_z.items():
            self.assertAlmostEqual(expected_rotate, rotateZUsd.Get(stage_time))

        # Check proxyShape2
        # make sure the session layer was properly grafted on
        refPrim2 = resultStage.GetPrimAtPath(refPrimPath2)
        self.assertTrue(refPrim2.IsValid())
        self.assertEqual(refPrim2.GetTypeName(), "Xform")
        self.assertEqual(refPrim2.GetSpecifier(), Sdf.SpecifierDef)
        
        refSpec2 = rootLayer.GetPrimAtPath(refPrimPath2)
        self.assertTrue(refSpec2)
        self.assertTrue(refSpec2.hasReferences)
        # ref root should be a defined xform on the main export layer also
        self.assertEqual(refSpec2.typeName, "Xform")
        self.assertEqual(refSpec2.specifier, Sdf.SpecifierDef)
        
        spherePrimPath = refPrimPath2 + secondSpherePath
        spherePrim = resultStage.GetPrimAtPath(spherePrimPath)
        self.assertTrue(spherePrim.IsValid())
        self.assertFalse(spherePrim.IsActive())
        # check that the proper specs are being created
        specOnExportLayer = rootLayer.GetPrimAtPath(spherePrimPath)
        self.assertEqual(specOnExportLayer.specifier, Sdf.SpecifierOver)

        os.remove(tempFile.name)

            
        
tests = unittest.TestLoader().loadTestsFromTestCase(TestTranslator)
result = unittest.TextTestRunner(verbosity=2).run(tests)

mc.quit(exitCode=(not result.wasSuccessful()))
