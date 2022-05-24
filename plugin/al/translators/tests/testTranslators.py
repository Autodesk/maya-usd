import unittest
import tempfile
import os
import maya.cmds as mc
import maya.mel as mel

from pxr import Tf, Usd, UsdGeom, Gf
import translatortestutils


class TestTranslator(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        mc.file(f=True, new=True)
        mc.loadPlugin('AL_USDMayaPlugin')
            
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
        mc.AL_usdmaya_ProxyShapeImport(file='./testMayaRef.usda')
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::MayaReference'))
        self.assertEqual(1, len(mc.ls('cube:pCube1')))
        self.assertEqual('mayaRefPrim', mc.listRelatives('cube:pCube1', parent=1)[0])
        self.assertEqual((0.0, 0.0, 0.0), mc.getAttr('mayaRefPrim.translate')[0])
        self.assertFalse(mc.getAttr('mayaRefPrim.translate', lock=1))
        self.assertEqual(1, len(mc.ls('otherNS:pCube1')))
        self.assertEqual('otherCube', mc.listRelatives('otherNS:pCube1', parent=1)[0])
        self.assertEqual((3.0, 2.0, 1.0), mc.getAttr('otherCube.translate')[0])
        self.assertFalse(mc.getAttr('otherCube.translate', lock=1))

        # Fallback namespace for prim without an explicit namespace
        expectedNamespace = 'test_cubeWithDefaultNamespace'
        self.assertTrue(mc.namespace(exists=expectedNamespace))
        self.assertEqual(1, len(mc.ls('%s:pCube1' % expectedNamespace)))

    def testMayaReference_RefLoading(self):
        '''Test that Maya References are only loaded when they need to be.'''
        import os
        import maya.api.OpenMaya as om
        import AL.usdmaya
             
        loadHistory = {}

        def recordRefLoad(refNodeMobj, mFileObject, clientData):
            '''Record when a reference path is loading.'''
            path = mFileObject.resolvedFullName()
            path = os.path.normpath(path)
            path = os.path.normcase(path)            
            count = loadHistory.setdefault(path, 0)
            loadHistory[path] = count + 1

        id = om.MSceneMessage.addReferenceCallback(om.MSceneMessage.kBeforeLoadReference, recordRefLoad)
             
        mc.file(new=1, f=1)
        proxyName = mc.AL_usdmaya_ProxyShapeImport(file='./testMayaRefLoading.usda')[0]
        proxy = AL.usdmaya.ProxyShape.getByName(proxyName)
        refPath = os.path.abspath('./cube.ma')
        refPath = os.path.normpath(refPath)
        refPath = os.path.normcase(refPath)        
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
        translatortestutils.importStageWithSphere()
              
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
        d = translatortestutils.importStageWithSphere()
        stage = d.stage
        mc.AL_usdmaya_TranslatePrim(ip="/pSphere1", fi=True, proxy="AL_usdmaya_Proxy")
      
        stage.SetEditTarget(stage.GetSessionLayer())
       
        # Delete some faces
        mc.select("pSphere1.f[0:399]", r=True)
        mc.select("pSphere1.f[0]", d=True)
        mc.delete()
       
        preSession = stage.GetEditTarget().GetLayer().ExportToString();
        mc.AL_usdmaya_TranslatePrim(tp="/pSphere1", proxy="AL_usdmaya_Proxy")
        postSession = stage.GetEditTarget().GetLayer().ExportToString()
               
        # Ensure data has been written
        sessionStage = Usd.Stage.Open(stage.GetEditTarget().GetLayer())
        sessionSphere = sessionStage.GetPrimAtPath("/pSphere1")

        self.assertTrue(sessionSphere.IsValid())
        self.assertTrue(sessionSphere.GetAttribute("faceVertexCounts"))

    def testMeshTranslator_variantswitch(self):
        mc.AL_usdmaya_ProxyShapeImport(file='./testMeshVariants.usda')
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::Mesh'))
        # test initial state has no meshes
        self.assertEqual(len(mc.ls(type='mesh')), 0)
    
        stage = translatortestutils.getStage()
        stage.SetEditTarget(stage.GetSessionLayer())
        variantPrim = stage.GetPrimAtPath("/TestVariantSwitch")
        variantSet = variantPrim.GetVariantSet("MeshVariants")
    
        # the MeshA should now be in the scene
        variantSet.SetVariantSelection("ShowMeshA")
        mc.AL_usdmaya_TranslatePrim(ip="/TestVariantSwitch/MeshA", fi=True, proxy="AL_usdmaya_Proxy") # force the import
        self.assertEqual(len(mc.ls('MeshA')), 1)
        self.assertEqual(len(mc.ls('MeshB')), 0)
        self.assertEqual(len(mc.ls(type='mesh')), 1)
        mc.AL_usdmaya_TranslatePrim(tp="/TestVariantSwitch/MeshA", proxy="AL_usdmaya_Proxy") # teardown

        #######
        # the MeshB should now be in the scene
        variantSet.SetVariantSelection("ShowMeshB")
        mc.AL_usdmaya_TranslatePrim(ip="/TestVariantSwitch/MeshB", fi=True, proxy="AL_usdmaya_Proxy") # force the import
        self.assertEqual(len(mc.ls('MeshA')), 0)
        self.assertEqual(len(mc.ls('MeshB')), 1)
        self.assertEqual(len(mc.ls(type='mesh')), 1)
        mc.AL_usdmaya_TranslatePrim(tp="/TestVariantSwitch/MeshB", proxy="AL_usdmaya_Proxy") # teardown

        # the MeshA and MeshB should be in the scene
        variantSet.SetVariantSelection("ShowMeshAnB")
        mc.AL_usdmaya_TranslatePrim(ip="/TestVariantSwitch/MeshB", fi=True, proxy="AL_usdmaya_Proxy")
        mc.AL_usdmaya_TranslatePrim(ip="/TestVariantSwitch/MeshA", fi=True, proxy="AL_usdmaya_Proxy")
 
        self.assertEqual(len(mc.ls('MeshA')), 1)
        self.assertEqual(len(mc.ls('MeshB')), 1)
        self.assertEqual(len(mc.ls(type='mesh')), 2)
 
        # switch variant to empty
        variantSet.SetVariantSelection("")
        self.assertEqual(len(mc.ls(type='mesh')), 0)

    def testNurbsCurve_TranslatorExists(self):
        """
        Test that the NurbsCurve Translator exists
        """
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::NurbsCurve'))

    def testNurbsCurve_TranslateOff(self):
        """
        Test that by default that the the mesh isn't imported
        """
        stage = translatortestutils.importStageWithNurbsCircle()
        self.assertEqual(len(mc.ls('nurbsCircle1')), 0)
        self.assertEqual(len(mc.ls(type='nurbsCurve')), 0)

    def testNurbsCurve_testTranslateOn(self):
        """
        Test that by default that the the mesh is imported
        """
        # setup scene with sphere
        translatortestutils.importStageWithNurbsCircle()

        # force the import
        mc.AL_usdmaya_TranslatePrim(ip="/nurbsCircle1", fi=True, proxy="AL_usdmaya_Proxy")

        self.assertEqual(len(mc.ls('nurbsCircle1')), 1)
        self.assertEqual(len(mc.ls(type='nurbsCurve')), 1)

    def testNurbsCurve_TranslateRoundTrip(self):
        """
        Test that Translating->TearingDown->Translating roundtrip works
        """
        # setup scene with nurbs circle
        # Create nurbs circle in Maya and export a .usda file
        mc.CreateNURBSCircle()
        mc.group('nurbsCircle1', name='parent')
        mc.select("parent")
        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="test_NurbsCurveTranslator_", delete=True)
        mc.file(tempFile.name, exportSelected=True, force=True, type="AL usdmaya export")

        # clear scene
        mc.file(f=True, new=True)
        mc.AL_usdmaya_ProxyShapeImport(file=tempFile.name)

        # force the import
        mc.AL_usdmaya_TranslatePrim(ip="/parent/nurbsCircle1", fi=True, proxy="AL_usdmaya_Proxy")
        self.assertEqual(len(mc.ls('nurbsCircle1')), 1)
        self.assertEqual(len(mc.ls(type='nurbsCurve')), 1)
        self.assertEqual(len(mc.ls('parent')), 1)

        # force the teardown
        mc.AL_usdmaya_TranslatePrim(tp="/parent/nurbsCircle1", fi=True, proxy="AL_usdmaya_Proxy")
        self.assertEqual(len(mc.ls('nurbsCircle1')), 0)
        self.assertEqual(len(mc.ls(type='nurbsCurve')), 0)
        self.assertEqual(len(mc.ls('parent')), 0)

        # force the import
        mc.AL_usdmaya_TranslatePrim(ip="/parent/nurbsCircle1", fi=True, proxy="AL_usdmaya_Proxy")
        self.assertEqual(len(mc.ls('nurbsCircle1')), 1)
        self.assertEqual(len(mc.ls(type='nurbsCurve')), 1)
        self.assertEqual(len(mc.ls('parent')), 1)

    def testNurbsCurve_curve_width_floatarray_export(self):
        expectedWidths = [1.,2.,3.]
        mc.CreateNURBSCircle()
        mc.select("nurbsCircleShape1")
        mc.addAttr(longName="width", dt="floatArray")
        mc.setAttr("nurbsCircleShape1.width", expectedWidths ,type="floatArray")
        self.assertEqual(mc.getAttr("nurbsCircleShape1.width"), expectedWidths)

        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="testNurbsCurve_curve_width_export", delete=False)
        tempFile.close()

        mc.AL_usdmaya_ExportCommand(file=tempFile.name)
        stage = Usd.Stage.Open(tempFile.name)
        prim = stage.GetPrimAtPath("/nurbsCircle1")
        nurbPrim = UsdGeom.NurbsCurves(prim)
        assert(nurbPrim.GetWidthsAttr())
        actualWidths = nurbPrim.GetWidthsAttr().Get()
        self.assertTrue(actualWidths)
        sure = any([a == b for a, b in zip(expectedWidths, actualWidths)])
        self.assertTrue(sure)

        os.remove(tempFile.name)

    def testNurbsCurve_curve_width_doublearray_export(self):
        expectedWidths = [1.,2.,3.]
        mc.CreateNURBSCircle()
        mc.select("nurbsCircleShape1")
        mc.addAttr(longName="width", dt="doubleArray")
        mc.setAttr("nurbsCircleShape1.width", expectedWidths ,type="doubleArray")
        self.assertEqual(mc.getAttr("nurbsCircleShape1.width"), expectedWidths)

        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="testNurbsCurve_curve_width_export", delete=False)
        tempFile.close()

        mc.AL_usdmaya_ExportCommand(file=tempFile.name)
        stage = Usd.Stage.Open(tempFile.name)
        prim = stage.GetPrimAtPath("/nurbsCircle1")
        nurbPrim = UsdGeom.NurbsCurves(prim)
        assert(nurbPrim.GetWidthsAttr())
        actualWidths = nurbPrim.GetWidthsAttr().Get()
        self.assertTrue(actualWidths)
        sure = any([a == b for a, b in zip(expectedWidths, actualWidths)])
        self.assertTrue(sure)

        os.remove(tempFile.name)

    def testNurbsCurve_curve_width_none_export(self):
        mc.CreateNURBSCircle()
        with self.assertRaises(ValueError):
            mc.getAttr("nurbsCircleShape1.width")

        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="testNurbsCurve_curve_width_export", delete=False)
        tempFile.close()

        mc.AL_usdmaya_ExportCommand(file=tempFile.name)
        stage = Usd.Stage.Open(tempFile.name)
        prim = stage.GetPrimAtPath("/nurbsCircle1")
        nurbPrim = UsdGeom.NurbsCurves(prim)
        self.assertTrue(nurbPrim.GetWidthsAttr())
        actualWidths = nurbPrim.GetWidthsAttr().Get()
        self.assertIsNone(actualWidths)

        os.remove(tempFile.name)

    def testNurbsCurve_curve_width_floatarrayempty_export(self):
        expectedWidths = []
        mc.CreateNURBSCircle()
        mc.select("nurbsCircleShape1")
        mc.addAttr(longName="width", dt="floatArray")
        mc.setAttr("nurbsCircleShape1.width", expectedWidths ,type="floatArray")
        self.assertIsNone(mc.getAttr("nurbsCircleShape1.width"))

        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="testNurbsCurve_curve_width_export", delete=False)
        tempFile.close()

        mc.AL_usdmaya_ExportCommand(file=tempFile.name)
        stage = Usd.Stage.Open(tempFile.name)
        prim = stage.GetPrimAtPath("/nurbsCircle1")
        nurbPrim = UsdGeom.NurbsCurves(prim)
        self.assertTrue(nurbPrim.GetWidthsAttr())
        actualWidths = nurbPrim.GetWidthsAttr().Get()
        self.assertEqual(len(actualWidths), 0)

        os.remove(tempFile.name)

    def testNurbsCurve_curve_width_doublearrayempty_export(self):
        expectedWidths = []
        mc.CreateNURBSCircle()
        mc.select("nurbsCircleShape1")
        mc.addAttr(longName="width", dt="doubleArray")
        mc.setAttr("nurbsCircleShape1.width", expectedWidths ,type="doubleArray")
        self.assertIsNone(mc.getAttr("nurbsCircleShape1.width"))

        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="testNurbsCurve_curve_width_export", delete=False)
        tempFile.close()

        mc.AL_usdmaya_ExportCommand(file=tempFile.name)
        stage = Usd.Stage.Open(tempFile.name)
        prim = stage.GetPrimAtPath("/nurbsCircle1")
        nurbPrim = UsdGeom.NurbsCurves(prim)
        self.assertTrue(nurbPrim.GetWidthsAttr())
        actualWidths = nurbPrim.GetWidthsAttr().Get()
        self.assertEqual(len(actualWidths), 0)

        os.remove(tempFile.name)

    def testNurbsCurve_curve_width_double_export(self):
        expectedWidths = 1.
        mc.CreateNURBSCircle()
        mc.select("nurbsCircleShape1")
        mc.addAttr(longName="width", at="double")
        mc.setAttr("nurbsCircleShape1.width", expectedWidths)
        self.assertEqual(mc.getAttr("nurbsCircleShape1.width"), expectedWidths)

        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="testNurbsCurve_curve_width_double_export", delete=False)
        tempFile.close()

        mc.AL_usdmaya_ExportCommand(file=tempFile.name)
        stage = Usd.Stage.Open(tempFile.name)
        prim = stage.GetPrimAtPath("/nurbsCircle1")
        nurbPrim = UsdGeom.NurbsCurves(prim)

        assert(nurbPrim.GetWidthsAttr())
        actualWidths = nurbPrim.GetWidthsAttr().Get()
        self.assertTrue(actualWidths)
        sure = any([a == b for a, b in zip([expectedWidths], actualWidths)])
        self.assertTrue(sure)

        os.remove(tempFile.name)

    def testNurbsCurve_curve_width_float_export(self):
        expectedWidths = 1.
        mc.CreateNURBSCircle()
        mc.select("nurbsCircleShape1")
        mc.addAttr(longName="width", at="float")
        mc.setAttr("nurbsCircleShape1.width", expectedWidths)
        self.assertEqual(mc.getAttr("nurbsCircleShape1.width"), expectedWidths)

        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="testNurbsCurve_curve_width_float_export", delete=False)
        tempFile.close()

        mc.AL_usdmaya_ExportCommand(file=tempFile.name)
        stage = Usd.Stage.Open(tempFile.name)
        prim = stage.GetPrimAtPath("/nurbsCircle1")
        nurbPrim = UsdGeom.NurbsCurves(prim)

        assert(nurbPrim.GetWidthsAttr())
        actualWidths = nurbPrim.GetWidthsAttr().Get()
        self.assertTrue(actualWidths)
        sure = any([a == b for a, b in zip([expectedWidths], actualWidths)])
        self.assertTrue(sure)

        os.remove(tempFile.name)

    def testNurbsCurve_PretearDownEditTargetWrite(self):
        """
        Simple test to determine if the edit target gets written to preteardown
        """

        # force the import
        stage = translatortestutils.importStageWithNurbsCircle()
        mc.AL_usdmaya_TranslatePrim(ip="/nurbsCircle1", fi=True, proxy="AL_usdmaya_Proxy")

        stage.SetEditTarget(stage.GetSessionLayer())

        # Delete a control vertex
        mc.select("nurbsCircle1.cv[6]", r=True)
        mc.delete()

        preSession = stage.GetEditTarget().GetLayer().ExportToString();
        mc.AL_usdmaya_TranslatePrim(tp="/nurbsCircle1", proxy="AL_usdmaya_Proxy")
        postSession = stage.GetEditTarget().GetLayer().ExportToString()

        # Ensure data has been written
        sessionStage = Usd.Stage.Open(stage.GetEditTarget().GetLayer())
        sessionNurbCircle = sessionStage.GetPrimAtPath("/nurbsCircle1")

        self.assertTrue(sessionNurbCircle.IsValid())
        cvcAttr = sessionNurbCircle.GetAttribute("curveVertexCounts")
        self.assertTrue(cvcAttr.IsValid())
        self.assertEqual(len(cvcAttr.Get()), 1)
        self.assertEqual(cvcAttr.Get()[0], 10)

    def testMeshTranslator_multipleTranslations(self):
        d = translatortestutils.importStageWithSphere('AL_usdmaya_Proxy')
        sessionLayer = d.stage.GetSessionLayer()
        d.stage.SetEditTarget(sessionLayer)

        spherePrimPath = "/"+d.sphereXformName
        offsetAmount = Gf.Vec3f(0,0.25,0)
        
        vertPoint = '{}.vtx[0]'.format(d.sphereXformName)
        
        spherePrimMesh = UsdGeom.Mesh.Get(d.stage, spherePrimPath)  

        # Test import,modify,teardown a bunch of times
        for i in range(3):
            # Determine expected result
            expectedPoint = spherePrimMesh.GetPointsAttr().Get()[0] + offsetAmount

            # Translate the prim into maya for editing
            mc.AL_usdmaya_TranslatePrim(forceImport=True, importPaths=spherePrimPath, proxy='AL_usdmaya_Proxy')

            # Move the point
            items = ['pSphere1.vtx[0]']
            mc.move(offsetAmount[0], offsetAmount[1], offsetAmount[2], items, relative=True) # just affect the Y
            mc.AL_usdmaya_TranslatePrim(teardownPaths=spherePrimPath, proxy='AL_usdmaya_Proxy')
 
            actualPoint = spherePrimMesh.GetPointsAttr().Get()[0]
            
            # Confirm that the edit has come back as expected
            self.assertAlmostEqual(actualPoint[0], expectedPoint[0])
            self.assertAlmostEqual(actualPoint[1], expectedPoint[1])
            self.assertAlmostEqual(actualPoint[2], expectedPoint[2])
           
    def testDirectionalLight_TranslatorExists(self):
        """
        Test that the Maya directional light Translator exists
        """
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::DirectionalLight')) 
    
    def testDirectionalLight_PluginIsFunctional(self):
        mc.AL_usdmaya_ProxyShapeImport(file='./testDirectionalLight.usda')
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::DirectionalLight'))
        self.assertEqual(len(mc.ls('directionalLightShape1')), 1)
        # Latest (2022-05-24) Maya preview release creates proxy light nodes in the scene for USD lights, 
        # so account for that as we verify the number of directional lights in the scene. 
        numProxyLights = len(mc.ls('ufeLightProxyShape*'))
        self.assertTrue((numProxyLights == 0) or (numProxyLights == 1))
        self.assertEqual(len(mc.ls(type='directionalLight')), 1 + numProxyLights)
        self.assertEqual('alight',mc.listRelatives(mc.listRelatives(mc.ls('directionalLightShape1')[0], parent=1)[0],parent=1)[0])

    def testDirectionalLight_TranslateRoundTrip(self):
        # setup scene with directional light
     
        # Create directional light in Maya and export a .usda file
        mel.eval('defaultDirectionalLight(3, 1,1,0, "0", 0,0,0, 0)')
        mc.setAttr('directionalLightShape1.lightAngle', 0.25)
        mc.setAttr('directionalLightShape1.pw', 6, 7 ,8)
        tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="test_DirectionalLightTranslator_", delete=False)
        tempFile.close()

        mc.AL_usdmaya_ExportCommand(file=tempFile.name)
              
        # clear scene
        mc.file(f=True, new=True)
         
        # import file back     
        mc.AL_usdmaya_ProxyShapeImport(file=tempFile.name)
        self.assertEqual(0.25, mc.getAttr('directionalLightShape1.lightAngle'))
        self.assertEqual((1.0, 1.0, 0), mc.getAttr('directionalLightShape1.color')[0])
        self.assertEqual(3, mc.getAttr('directionalLightShape1.intensity'))
        self.assertEqual((6.0, 7.0, 8.0), mc.getAttr('directionalLightShape1.pointWorld')[0])

        os.remove(tempFile.name)

    def testFrameRange_TranslatorExists(self):
        """
        Test that the Maya frame range Translator exists
        """
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::FrameRange'))
        
    def testFrameRange_PluginIsFunctional(self):
        mc.AL_usdmaya_ProxyShapeImport(file='./testFrameRange.usda')
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::FrameRange'))
        
        currentFrame = mc.currentTime(q=True)
        
        startAnimFrame = mc.playbackOptions(q=True, animationStartTime=True)
        endAnimFrame = mc.playbackOptions(q=True, animationEndTime=True)
        
        startVisibleFrame = mc.playbackOptions(q=True, minTime=True)
        endVisibleFrame = mc.playbackOptions(q=True, maxTime=True)
        
        
        self.assertEqual(currentFrame, 1100)
        
        self.assertEqual(startAnimFrame, 1072)
        self.assertEqual(endAnimFrame, 1290)
        
        self.assertEqual(startVisibleFrame, 1080)
        self.assertEqual(endVisibleFrame, 1200)
        
    def testFrameRange_FallbackIsFunctional(self):
        # If no frame range data is authored in ALFrameRange prim, we use startTimeCode & endTimeCode from usdStage:
        mc.AL_usdmaya_ProxyShapeImport(file='./testFrameRangeFallback.usda')
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::FrameRange'))
        
        currentFrame = mc.currentTime(q=True)
        
        startAnimFrame = mc.playbackOptions(q=True, animationStartTime=True)
        endAnimFrame = mc.playbackOptions(q=True, animationEndTime=True)
        
        startVisibleFrame = mc.playbackOptions(q=True, minTime=True)
        endVisibleFrame = mc.playbackOptions(q=True, maxTime=True)
        
        self.assertEqual(currentFrame, 1072)
        
        self.assertEqual(startAnimFrame, 1072)
        self.assertEqual(endAnimFrame, 1290)
        
        self.assertEqual(startVisibleFrame, 1072)
        self.assertEqual(endVisibleFrame, 1290)
        
    def testFrameRange_OnlyCurrentFrame(self):
        startAnimFrameOld = mc.playbackOptions(q=True, animationStartTime=True)        
        endAnimFrameOld = mc.playbackOptions(q=True, animationEndTime=True)
        
        startVisibleFrameOld = mc.playbackOptions(q=True, minTime=True)
        endVisibleFrameOld = mc.playbackOptions(q=True, maxTime=True)
        
        # If frame range data is authored neither in ALFrameRange prim nor usdStage, no range action should be taken:
        mc.AL_usdmaya_ProxyShapeImport(file='./testFrameRangeCurrentFrame.usda')
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::FrameRange'))
        
        currentFrame = mc.currentTime(q=True)
        
        startAnimFrame = mc.playbackOptions(q=True, animationStartTime=True)
        endAnimFrame = mc.playbackOptions(q=True, animationEndTime=True)
        
        startVisibleFrame = mc.playbackOptions(q=True, minTime=True)
        endVisibleFrame = mc.playbackOptions(q=True, maxTime=True)
        
        self.assertEqual(currentFrame, 10)
        
        self.assertEqual(startAnimFrame, startAnimFrameOld)
        self.assertEqual(endAnimFrame, endAnimFrameOld)
        
        self.assertEqual(startVisibleFrame, startVisibleFrameOld)
        self.assertEqual(endVisibleFrame, endVisibleFrameOld)
        
    def testFrameRange_NoImpactIfNoFrameRange(self):
        currentFrameOld = mc.currentTime(q=True)
        
        startAnimFrameOld = mc.playbackOptions(q=True, animationStartTime=True)        
        endAnimFrameOld = mc.playbackOptions(q=True, animationEndTime=True)
        
        startVisibleFrameOld = mc.playbackOptions(q=True, minTime=True)
        endVisibleFrameOld = mc.playbackOptions(q=True, maxTime=True)
        
        # If frame range data is authored neither in ALFrameRange prim nor usdStage, no range action should be taken:
        mc.AL_usdmaya_ProxyShapeImport(file='./testFrameRangeNoImpact.usda')
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::FrameRange'))
        
        currentFrame = mc.currentTime(q=True)
        
        startAnimFrame = mc.playbackOptions(q=True, animationStartTime=True)
        endAnimFrame = mc.playbackOptions(q=True, animationEndTime=True)
        
        startVisibleFrame = mc.playbackOptions(q=True, minTime=True)
        endVisibleFrame = mc.playbackOptions(q=True, maxTime=True)
        
        self.assertEqual(currentFrame, currentFrameOld)
        
        self.assertEqual(startAnimFrame, startAnimFrameOld)
        self.assertEqual(endAnimFrame, endAnimFrameOld)
        
        self.assertEqual(startVisibleFrame, startVisibleFrameOld)
        self.assertEqual(endVisibleFrame, endVisibleFrameOld)
    
    # --------------------------------------------------------------------------------------------------    
    def _performDisablePrimTest(self, usdFilePath):
        shapes = mc.AL_usdmaya_ProxyShapeImport(file=usdFilePath)
        self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::FrameRange'))
        
        shotPrimMayaNodeName = 'shot_name'
        self.assertTrue(mc.objExists(shotPrimMayaNodeName))
        configPrimPath = '/shot_name/config'
        mc.AL_usdmaya_ActivatePrim(shapes[0], pp=configPrimPath, a=False)
        
        # Assert not clearing:
        self.assertTrue(mc.objExists(shotPrimMayaNodeName))
        
    def testTranslatedPrimDeactiveClearIssue(self):
        # When a prim is within a translated hierarchy, deactivating it's parent should not clear children of proxyShapeTransform.
        self._performDisablePrimTest('./testTranslatedPrimDisableClearIssue.usda')
        
    def testTranslatedPrimDeactiveCrashIssue(self):
        # When a prim is within a translated hierarchy, deactivating it's parent should not crash Maya or clear the children of proxyShapeTransform.
        self._performDisablePrimTest('./testTranslatedPrimDisableCrashIssue.usda')
        
        
tests = unittest.TestLoader().loadTestsFromTestCase(TestTranslator)
result = unittest.TextTestRunner(verbosity=2).run(tests)

mc.quit(exitCode=(not result.wasSuccessful()))
