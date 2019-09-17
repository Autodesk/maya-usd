import unittest


from maya import cmds

from AL import usdmaya

from pxr import Usd, UsdUtils, Tf

class CubeGenerator(usdmaya.TranslatorBase):
    '''
    Basic Translator which doesn't support update
    '''
    initializeCount = 0
    preTearDownCount = 0
    tearDownCount = 0
    postImportCount = 0
    importObjectCount = 0
    updateCount = 0
    importObjectMObjects = []
    
    @classmethod
    def resetState(cls):
        cls.initializeCount = 0
        cls.preTearDownCount = 0
        cls.tearDownCount = 0
        cls.postImportCount = 0
        cls.importObjectCount = 0
        cls.updateCount = 0
        cls.importObjectMObjects = []
        
    @classmethod
    def getState(cls):
        return {"initializeCount": cls.initializeCount, 
                "preTearDownCount": cls.preTearDownCount, 
                "tearDownCount": cls.tearDownCount, 
                "postImportCount":cls.postImportCount,
                "importObjectCount":cls.importObjectCount,
                "updateCount":cls.updateCount,
                "importObjectMObjects":cls.importObjectMObjects  }

    def initialize(self):
        return True
    
    def preTearDown(self, prim):
        self.__class__.preTearDownCount +=1
        return True
    
    def tearDown(self, path):
        self.__class__.tearDownCount +=1
        self.removeItems(path)
        return True
    
    def canExport(self, mayaObjectName):
        return False
    
    def needsTransformParent(self):
        return True
    
    def supportsUpdate(self):
        return False
    
    def importableByDefault(self):
        return True
    
    def exportObject(self, stage, path, usdPath, params):
        return
    
    def postImport(self, prim):
        return True

    def getTranslatedType(self):
        return Tf.Type.Unknown
       

    def importObject(self, prim, parent=None):
        self.__class__.importObjectCount +=1
        numCubesAttr = prim.GetAttribute("numCubes")
        numCubes = 0
        if numCubesAttr.IsValid():
            numCubes = numCubesAttr.Get()

        dgNodes = []
        for x in range(numCubes):
            nodes = cmds.polyCube()
            dgNodes.append(nodes[1])
            cmds.parent(nodes[0], parent)

        # Only insert the DG nodes. The Dag nodes
        # created under the parent will be deleted
        # by AL_USDMaya automatically when the parent
        # transform is deleted.
        for n in dgNodes:
            self.insertItem(prim, n)

        self.__class__.importObjectMObjects = self.context().getMObjectsPath(prim)
        return True
    

    def update(self, prim):
        self.__class__.updateCount +=1
        return True


class DeleteParentNodeOnPostImport(usdmaya.TranslatorBase):
    '''
    Translator that deletes the parent node on postImport.
    '''

    nbImport = 0
    nbPostImport = 0
    nbPreTeardown = 0

    parentNode = None

    def initialize(self):
        return True

    def preTearDown(self, prim):
        self.__class__.nbPreTeardown += 1
        return True

    def tearDown(self, path):
        self.removeItems()
        return True

    def needsTransformParent(self):
        return True

    def importableByDefault(self):
        return True

    def postImport(self, prim):
        self.__class__.nbPostImport += 1
        if self.__class__.parentNode:
            cmds.delete(self.__class__.parentNode)
            # re-create the node
            # print 'CCCCCCCCCCCCc', cmds.createNode('AL_usdmaya_Transform', name='rig', parent='|bobo|root|peter01')

        return True

    def getTranslatedType(self):
        return Tf.Type.Unknown

    def importObject(self, prim, parent=None):
        self.__class__.nbImport += 1
        self.__class__.parentNode = parent
        return True

    def supportsUpdate(self):
        return False

    def update(self, prim):
        return True

    def canExport(self, mayaObjectName):
        return False

    def exportObject(self, stage, path, usdPath, params):
        return


class TestPythonTranslators(unittest.TestCase):
    
    def setUp(self):
        cmds.file(force=True, new=True)
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)
        self.assertTrue(cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True))
        
        
    def tearDown(self):
        CubeGenerator.resetState()
        UsdUtils.StageCache.Get().Clear()
        usdmaya.TranslatorBase.clearTranslators()
    
    def test_registration(self):
        import examplecubetranslator  #This registers the translator

        self.assertTrue(len(usdmaya.TranslatorBase.getPythonTranslators())==1)

        usdmaya.TranslatorBase.registerTranslator(examplecubetranslator.BoxNodeTranslator())
        self.assertTrue(len(usdmaya.TranslatorBase.getPythonTranslators())==2)  #at the moment we allow duplicates
        usdmaya.TranslatorBase.clearTranslators()

        self.assertTrue(len(usdmaya.TranslatorBase.getPythonTranslators())==0) #check empty

    def test_import(self):

        usdmaya.TranslatorBase.registerTranslator(CubeGenerator(), 'beast_rig')

        stage = Usd.Stage.Open("../test_data/inactivetest.usda")
        prim = stage.GetPrimAtPath('/root/peter01')
        vs = prim.GetVariantSet("cubes")
        vs.SetVariantSelection("fiveCubes")

        stageCache = UsdUtils.StageCache.Get()
        stageCache.Insert(stage)
        stageId = stageCache.GetId(stage)

        cmds.AL_usdmaya_ProxyShapeImport(stageId=stageId.ToLongInt(), name='bobo')
        self.assertTrue(CubeGenerator.getState()["importObjectCount"]==1)
        self.assertTrue(len(CubeGenerator.importObjectMObjects)==5)

        prim = stage.GetPrimAtPath('/root/peter01/rig')
        # self.assertTrue(usdmaya.TranslatorBase.generateTranslatorId(prim)=="assettype:beast_rig")

    def test_variantSwitch_that_removes_prim_and_create_new_one(self):

        usdmaya.TranslatorBase.registerTranslator(CubeGenerator(), 'beast_rig')

        stage = Usd.Stage.Open("../test_data/inactivetest.usda")
        prim = stage.GetPrimAtPath('/root/peter01')
        vs = prim.GetVariantSet("cubes")
        vs.SetVariantSelection("fiveCubes")

        stageCache = UsdUtils.StageCache.Get()
        stageCache.Insert(stage)
        stageId = stageCache.GetId(stage)

        cmds.AL_usdmaya_ProxyShapeImport(stageId=stageId.ToLongInt(), name='bobo')
        self.assertEqual(CubeGenerator.getState()["importObjectCount"],1)
        self.assertEqual(len(CubeGenerator.importObjectMObjects),5)
        self.assertTrue(cmds.objExists('|bobo|root|peter01|rig'))

        '''
        Variant switch that leads to another prim being created.
        '''
        vs.SetVariantSelection("sixCubesRig2")

        self.assertEqual(CubeGenerator.getState()["tearDownCount"],1)
        self.assertEqual(CubeGenerator.getState()["importObjectCount"],2)
        self.assertEqual(CubeGenerator.getState()["updateCount"],0)
        self.assertEqual(len(CubeGenerator.importObjectMObjects),6)
        self.assertFalse(cmds.objExists('|bobo|root|peter01|rig'))
        self.assertTrue(cmds.objExists('|bobo|root|peter01|rig2'))

    def test_variantSwitch_that_removes_prim_runs_teardown(self):

        usdmaya.TranslatorBase.registerTranslator(CubeGenerator(), 'beast_rig')

        stage = Usd.Stage.Open("../test_data/inactivetest.usda")
        prim = stage.GetPrimAtPath('/root/peter01')
        vs = prim.GetVariantSet("cubes")
        vs.SetVariantSelection("fiveCubes")

        stageCache = UsdUtils.StageCache.Get()
        stageCache.Insert(stage)
        stageId = stageCache.GetId(stage)

        cmds.AL_usdmaya_ProxyShapeImport(stageId=stageId.ToLongInt(), name='bobo')
        self.assertEqual(CubeGenerator.getState()["importObjectCount"],1)
        self.assertTrue(cmds.objExists('|bobo|root|peter01|rig'))

        '''
        Test that we can swap in another empty variant and our content gets deleted
        '''
        vs.SetVariantSelection("noCubes")

        self.assertEqual(CubeGenerator.getState()["tearDownCount"], 1)
        self.assertEqual(CubeGenerator.getState()["importObjectCount"], 1)
        self.assertFalse(cmds.objExists('|bobo|root|peter01|rig'))

    def test_variantSwitch_that_keeps_existing_prim_runs_teardown_and_import(self):
        usdmaya.TranslatorBase.registerTranslator(CubeGenerator(), 'beast_rig')

        stage = Usd.Stage.Open("../test_data/inactivetest.usda")
        prim = stage.GetPrimAtPath('/root/peter01')
        vs = prim.GetVariantSet("cubes")
        vs.SetVariantSelection("fiveCubes")

        stageCache = UsdUtils.StageCache.Get()
        stageCache.Insert(stage)
        stageId = stageCache.GetId(stage)

        cmds.AL_usdmaya_ProxyShapeImport(stageId=stageId.ToLongInt(), name='bobo')

        self.assertEqual(CubeGenerator.getState()["importObjectCount"],1)
        self.assertEqual(len(CubeGenerator.importObjectMObjects),5)
        self.assertTrue(cmds.objExists('|bobo|root|peter01|rig'))

        # check the number of items under the parent. We should have 6 created nodes
        parentPath = '|bobo|root|peter01|rig'
        self.assertTrue(cmds.objExists(parentPath))
        nbItems = cmds.listRelatives(parentPath)
        self.assertEqual(5, len(nbItems))

        '''
        Variant switch that leads to same prim still existing.
        '''
        vs.SetVariantSelection("sixCubesRig")

        self.assertEqual(CubeGenerator.getState()["tearDownCount"],1)
        self.assertEqual(CubeGenerator.getState()["importObjectCount"],2)
        self.assertEqual(CubeGenerator.getState()["updateCount"],0)
        self.assertEqual(len(CubeGenerator.importObjectMObjects),6)

        # check the number of items under the parent. We should have 6 created nodes
        # and not 11 as teardown is supposed to have run and removed the old ones.
        self.assertTrue(cmds.objExists(parentPath))
        nbItems = cmds.listRelatives(parentPath)
        self.assertEqual(6, len(nbItems))

    def test_set_inactive_prim_removes_parent_transform(self):
        usdmaya.TranslatorBase.registerTranslator(CubeGenerator(), 'beast_rig')

        stage = Usd.Stage.Open("../test_data/inactivetest.usda")
        prim = stage.GetPrimAtPath('/root/peter01')
        vs = prim.GetVariantSet("cubes")
        vs.SetVariantSelection("fiveCubes")

        stageCache = UsdUtils.StageCache.Get()
        stageCache.Insert(stage)
        stageId = stageCache.GetId(stage)

        cmds.AL_usdmaya_ProxyShapeImport(stageId=stageId.ToLongInt(), name='bobo')

        self.assertEqual(CubeGenerator.getState()["importObjectCount"],1)
        self.assertEqual(len(CubeGenerator.importObjectMObjects),5)
        self.assertTrue(cmds.objExists('|bobo|root|peter01|rig'))

        p = stage.GetPrimAtPath('/root/peter01/rig')
        p.SetActive(False)

        self.assertEqual(CubeGenerator.getState()["tearDownCount"],1)
        self.assertEqual(CubeGenerator.getState()["importObjectCount"],1)
        self.assertEqual(CubeGenerator.getState()["updateCount"],0)

        self.assertFalse(cmds.objExists('|bobo|root|peter01|rig'))

    # this test is in progress... I cannot make it fail currently but
    # the motion translator in unicorn is definitely crashing Maya
    # if needsTransformParent() returns True.
    def test_deletion_of_parent_node_by_translator_does_not_crash_Maya(self):
        usdmaya.TranslatorBase.registerTranslator(DeleteParentNodeOnPostImport(), 'beast_rig')

        stage = Usd.Stage.Open("../test_data/inactivetest.usda")
        prim = stage.GetPrimAtPath('/root/peter01')
        vs = prim.GetVariantSet("cubes")
        vs.SetVariantSelection("fiveCubes")

        stageCache = UsdUtils.StageCache.Get()
        stageCache.Insert(stage)
        stageId = stageCache.GetId(stage)

        cmds.AL_usdmaya_ProxyShapeImport(stageId=stageId.ToLongInt(), name='bobo')

        self.assertEqual(DeleteParentNodeOnPostImport.nbImport, 1)
        self.assertEqual(DeleteParentNodeOnPostImport.parentNode, '|bobo|root|peter01|rig')
        self.assertEqual(DeleteParentNodeOnPostImport.nbPostImport, 1)
        self.assertEqual(DeleteParentNodeOnPostImport.nbPreTeardown, 0)

        vs.SetVariantSelection("noCubes")


if __name__ == "__main__":

    tests = [unittest.TestLoader().loadTestsFromTestCase(TestPythonTranslators)]
    
    results = [unittest.TextTestRunner(verbosity=2).run(test) for test in tests]
    exitCode = int(not all([result.wasSuccessful() for result in results]))
    cmds.quit(force=True, exitCode=(exitCode))
