import unittest
import tempfile
import maya.standalone
import maya.cmds as mc

from pxr import Tf, Usd

class testTranslator(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        maya.standalone.initialize()
        mc.loadPlugin('AL_USDMayaPlugin')
            
    @classmethod
    def tearDownClass(cls):
        maya.standalone.uninitialize()
    
    @classmethod
    def tearDown(cls):
        mc.file(f=True, new=True)
            
    def testMayaReference_TranslatorExists(self):
        """
        Test that the Maya Reference Translator exists
        """
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::MayaReference'))
    
    def testMayaReference_TranslatorImport(self):
        """
        Test that the Maya Reference Translator imports correctly
        """
            
        mc.AL_usdmaya_ProxyShapeImport(file='./testMayaRef.usda')
        self.assertTrue(len(mc.ls('cube:pCube1')))
    
    def testMayaReference_PluginIsFunctional(self):
        mc.file(new=1, f=1)
        mc.AL_usdmaya_ProxyShapeImport(file='./testMayaRef.usda')
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::MayaReference'))
        self.assertEqual(1, len(mc.ls('cube:pCube1')))
        self.assertEqual('mayaRefPrim', mc.listRelatives('cube:pCube1', parent=1)[0])
        self.assertEqual((0.0, 0.0, 0.0), cmds.getAttr('mayaRefPrim.translate')[0])
        self.assertFalse(mc.getAttr('mayaRefPrim.translate', lock=1))
        self.assertEqual(1, len(mc.ls('otherNS:pCube1')))
        self.assertEqual('otherCube', mc.listRelatives('otherNS:pCube1', parent=1)[0])
        self.assertEqual((3.0, 2.0, 1.0), cmds.getAttr('otherCube.translate')[0])
        self.assertFalse(mc.getAttr('otherCube.translate', lock=1))
    
    def testMayaReference_RefLoading(self):
        '''Test that Maya References are only loaded when they need to be.'''
        import os
        import maya.api.OpenMaya as om
        import AL.usdmaya
            
        loadHistory = {}
            
    def recordRefLoad(refNodeMobj, mFileObject, clientData):
        '''Record when a reference path is loading.'''
        path = mFileObject.resolvedFullName()
        count = loadHistory.setdefault(path, 0)
        loadHistory[path] = count + 1
            
        id = om.MSceneMessage.addReferenceCallback(
        om.MSceneMessage.kBeforeLoadReference,
        recordRefLoad)
            
        mc.file(new=1, f=1)
        proxyName = mc.AL_usdmaya_ProxyShapeImport(file='./testMayaRefLoading.usda')[0]
        proxy = AL.usdmaya.ProxyShape.getByName(proxyName)
        refPath = os.path.abspath('./cube.ma')
        stage = AL.usdmaya.StageCache.Get().GetAllStages()[0]
        topPrim = stage.GetPrimAtPath('/world/optionalCube')
        loadVariantSet = topPrim.GetVariantSet('state')
            
        self.assertEqual(loadHistory[refPath], 2)  # one for each copy of ref
            
        proxy.resync('/')
        self.assertEqual(loadHistory[refPath], 2)  # refs should not have reloaded since nothing has changed.
            
        # now change the variant, so a third ALMayaReference should load
        loadVariantSet.SetVariantSelection('loaded')
        self.assertEqual(loadHistory[refPath], 3)  # only the new ref should load.
            
        loadVariantSet.SetVariantSelection('unloaded')
        self.assertEqual(loadHistory[refPath], 3)  # ref should unload, but nothing else should load.
            
        om.MMessage.removeCallback(id)
            
    def testMayaReference_RefKeepsRefEdits(self):
        '''Tests that, even if the MayaReference is swapped for a pure-usda representation, refedits are preserved'''
        import AL.usdmaya
            
        mc.file(new=1, f=1)
        mc.AL_usdmaya_ProxyShapeImport(file='./testMayaRefUnloadable.usda')
        stage = AL.usdmaya.StageCache.Get().GetAllStages()[0]
        topPrim = stage.GetPrimAtPath('/usd_top')
        mayaLoadingStyle = topPrim.GetVariantSet('mayaLoadingStyle')
        
    def assertUsingMayaReferenceVariant():
        self.assertEqual(1, len(mc.ls('cubeNS:pCube1')))
        self.assertFalse(stage.GetPrimAtPath('/usd_top/pCube1_usd').IsValid())
        self.assertEqual('mayaReference', mayaLoadingStyle.GetVariantSelection())
        
    def assertUsingUsdVariant():
        self.assertEqual(0, len(mc.ls('cubeNS:pCube1')))
        self.assertTrue(stage.GetPrimAtPath('/usd_top/pCube1_usd').IsValid())
        self.assertEqual('usd', mayaLoadingStyle.GetVariantSelection())
            
        # by default, variant should be set so that it's a maya reference
        assertUsingMayaReferenceVariant()
            
        # check that, by default, the cube is at 1,2,3
        self.assertEqual(mc.getAttr('cubeNS:pCube1.translate')[0], (1.0, 2.0, 3.0))
        # now make a ref edit
        mc.setAttr('cubeNS:pCube1.translate', 4.0, 5.0, 6.0)
        self.assertEqual(mc.getAttr('cubeNS:pCube1.translate')[0], (4.0, 5.0, 6.0))
            
        # now change the variant, so the ALMayaReference isn't part of the stage anymore
        mayaLoadingStyle.SetVariantSelection('usd')
        # confirm that worked
        assertUsingUsdVariant()
            
        # now switch back...
        mayaLoadingStyle.SetVariantSelection('mayaReference')
        assertUsingMayaReferenceVariant()
        # ...and then make sure that our ref edit was preserved
        self.assertEqual(mc.getAttr('cubeNS:pCube1.translate')[0], (4.0, 5.0, 6.0))

 
    def testMesh_TranslatorExists(self):
        """
        Test that the Maya Reference Translator exists
        """
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::Mesh'))


    def testMesh_TranslateOff(self):
        """
        Test that by default that the the mesh isn't imported
        """
      
        # Create sphere in Maya and export a .usda file
        mc.polySphere()
        mc.select("pSphere1")
        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="test_MeshTranslator_", delete=True)
        mc.file(tempFile.name, exportSelected=True, force=True, type="AL usdmaya export")
              
        # clear scene
        mc.file(f=True, new=True)
              
        mc.AL_usdmaya_ProxyShapeImport(file=tempFile.name)
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::Mesh'))
        self.assertEqual(len(mc.ls('pSphere1')), 0)
        self.assertEqual(len(mc.ls(type='mesh')), 0)

    def testMesh_testTranslateOn(self):
        """
        Test that by default that the the mesh is imported
        """
        # setup scene with sphere
        self._importStageWithSphere()
             
        # force the import
        mc.AL_usdmaya_TranslatePrim(ip="/pSphere1", fi=True, proxy="AL_usdmaya_Proxy")
     
        self.assertTrue(len(mc.ls('pSphere1')))
        self.assertEqual(len(mc.ls(type='mesh')), 1)
   
    def testMesh_TranslateRoundTrip(self):
        """
        Test that Translating->TearingDown->Translating roundtrip works
        """
        # setup scene with sphere
    
        # Create sphere in Maya and export a .usda file
        mc.polySphere()
        mc.group( 'pSphere1', name='parent' )
        mc.select("parent")
        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="test_MeshTranslator_", delete=True)
        mc.file(tempFile.name, exportSelected=True, force=True, type="AL usdmaya export")
             
        # clear scene
        mc.file(f=True, new=True)
             
        mc.AL_usdmaya_ProxyShapeImport(file=tempFile.name)
            
    
        # force the import
        mc.AL_usdmaya_TranslatePrim(ip="/parent/pSphere1", fi=True, proxy="AL_usdmaya_Proxy")
        self.assertEqual(len(mc.ls('pSphere1')), 1)
        self.assertEqual(len(mc.ls(type='mesh')), 1)
        self.assertEqual(len(mc.ls('parent')), 1)
    
        # force the teardown 
        mc.AL_usdmaya_TranslatePrim(tp="/parent/pSphere1", fi=True, proxy="AL_usdmaya_Proxy")
        self.assertEqual(len(mc.ls('pSphere1')), 0)
        self.assertEqual(len(mc.ls(type='mesh')), 0)
        self.assertEqual(len(mc.ls('parent')), 0)
    
        # force the import
        mc.AL_usdmaya_TranslatePrim(ip="/parent/pSphere1", fi=True, proxy="AL_usdmaya_Proxy")
        self.assertEqual(len(mc.ls('pSphere1')), 1)
        self.assertEqual(len(mc.ls(type='mesh')), 1)
        self.assertEqual(len(mc.ls('parent')), 1)
             
     
    def testMesh_PretearDownEditTargetWrite(self):
        """
        Simple test to determine if the edit target gets written to preteardown 
        """
              
        # force the import
        stage = self._importStageWithSphere()
        mc.AL_usdmaya_TranslatePrim(ip="/pSphere1", fi=True, proxy="AL_usdmaya_Proxy")
     
        stage.SetEditTarget(stage.GetSessionLayer())
      
        # Delete some faces
        mc.select("pSphere1.f[0:399]", r=True)
        mc.select("pSphere1.f[0]", d=True)
        mc.delete()
      
        preSession = stage.GetEditTarget().GetLayer().ExportToString();
        mc.AL_usdmaya_TranslatePrim(tp="/pSphere1", proxy="AL_usdmaya_Proxy")
        postSession = stage.GetEditTarget().GetLayer().ExportToString()
              
        # Introspect the layer to determine if data has been written
        sessionStage = Usd.Stage.Open(stage.GetEditTarget().GetLayer())
        sessionSphere = sessionStage.GetPrimAtPath("/pSphere1")
        self.assertTrue(sessionSphere.IsValid())
        self.assertTrue(sessionSphere.GetAttribute("faceVertexCounts"))

### Commented since there is a crash when out of a variant that contains a mesh into a variant that contains a different mesh.
### Possibly because we are changing the Session Layer in the preTearDown of the Mesh.cpp while in a variantSetChanged callback.
#     def testMeshTranslator_variantswitch(self):
#         mc.AL_usdmaya_ProxyShapeImport(file='./testMeshVariants.usda')
#         self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::Mesh'))
#    
#         # test initial state has no meshes
#         self.assertEqual(len(mc.ls(type='mesh')), 0)
#  
#         stage = self._getStage()
#         stage.SetEditTarget(stage.GetSessionLayer())
#          
#         variantPrim = stage.GetPrimAtPath("/TestVariantSwitch")
#         variantSet = variantPrim.GetVariantSet("MeshVariants")
#  
#         # the MeshA should now be in the scene
#         variantSet.SetVariantSelection("ShowMeshA")
#         mc.AL_usdmaya_TranslatePrim(ip="/TestVariantSwitch/MeshA", fi=True, proxy="AL_usdmaya_Proxy") # force the import
#         self.assertEqual(len(mc.ls('MeshA')), 1)
#         self.assertEqual(len(mc.ls('MeshB')), 0)
#         self.assertEqual(len(mc.ls(type='mesh')), 1)
#  
#         ########
#         # FIXUP When it selects the second variant, in this case ShowMeshB, it crashes in 2017 with the following trace
#         #0  TpolyGrp::findGrp (this=<optimized out>, grpNumber=<optimized out>, type=<optimized out>) at /data2/builds/animallogic2017/client/src/PolyEngine/include/TpolyGrp.h:210
#         #1  TpolyGeom::findGrp (this=<optimized out>, name=<optimized out>, type=<optimized out>) at /data2/builds/animallogic2017/client/src/PolyEngine/include/TpolyInline.h:476
#         #2  TpolyGeom::getHoleFaces (this=0xf1a8410, faceIds=...) at /data2/builds/animallogic2017/client/src/PolyEngine/PolyGeom/TpolyGeomHoleFace.cpp:66
#         #3  0x00007f1a4a6fc633 in MFnMesh::getInvisibleFaces (this=this@entry=0x7fff3b656e70, ReturnStatus=ReturnStatus@entry=0x0) at /data2/builds/animallogic2017/client/src/OpenMaya/Polys/MFnMesh.cpp:8881
#         #4  0x00007f19a6bcd7ce in AL::usdmaya::utils::copyInvisibleHoles (mesh=..., fnMesh=...) at AL/usdmaya/utils/MeshUtils.cpp:1241
#         # Previously I was noticing some USD thread issue where it was accessing a layer from different threads.
#         #######
#         # the MeshB should now be in the scene
#         variantSet.SetVariantSelection("ShowMeshB")
#         #print stage.ExportToString()
#         mc.AL_usdmaya_TranslatePrim(ip="/TestVariantSwitch/MeshB", fi=True, proxy="AL_usdmaya_Proxy") # force the import
#         self.assertEqual(len(mc.ls('MeshA')), 0)
#         self.assertEqual(len(mc.ls('MeshB')), 1)
#         self.assertEqual(len(mc.ls(type='mesh')), 1)
#  
#         # the MeshA and MeshB should be in the scene
#         variantSet.SetVariantSelection("ShowMeshAnB")
#         mc.AL_usdmaya_TranslatePrim(ip="/TestVariantSwitch/MeshB", fi=True, proxy="AL_usdmaya_Proxy") # force the import
#         mc.AL_usdmaya_TranslatePrim(ip="/TestVariantSwitch/MeshA", fi=True, proxy="AL_usdmaya_Proxy") # force the import
#  
#         self.assertEqual(len(mc.ls('MeshA')), 1)
#         self.assertEqual(len(mc.ls('MeshB')), 1)
#         self.assertEqual(len(mc.ls(type='mesh')), 2)
#  
#         # switch variant to empty
#         variantSet.SetVariantSelection("")
#         self.assertEqual(len(mc.ls(type='mesh')), 0)

 

# Utilities
    def _getStage(self):
        from AL import usdmaya
        stageCache = usdmaya.StageCache.Get()
        stage = stageCache.GetAllStages()[0]
        return stage
    
    def _importStageWithSphere(self):
        """
        Creates Scene

        #usda1.0
        def Mesh "pSphere1"()
        {
        }
        """

        # Create sphere in Maya and export a .usda file
        mc.polySphere()
        mc.select("pSphere1")
        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="test_MeshTranslator_", delete=True)
        mc.file(tempFile.name, exportSelected=True, force=True, type="AL usdmaya export")
         
        # clear scene
        mc.file(f=True, new=True)
        mc.AL_usdmaya_ProxyShapeImport(file=tempFile.name)
        return self._getStage()

if __name__ == '__main__':
    unittest.main()
