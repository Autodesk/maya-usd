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
        mc.setAttr(proxyShapeNode + ".timeOffset", 40)
        mc.setAttr(proxyShapeNode + ".timeScalar", 2)
        
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
        self.assertEqual(refs[0].layerOffset, Sdf.LayerOffset(40, 2))
        
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
