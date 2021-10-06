#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

import os
import sys
import unittest
import tempfile

from maya import cmds

from AL import usdmaya

from pxr import Usd, UsdUtils, Tf

import fixturesUtils

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

class UpdateableTranslator(usdmaya.TranslatorBase):

    def initialize(self):
        self.actions = []
        return True

    def getTranslatedType(self):
        return Tf.Type.Unknown

    def needsTransformParent(self):
        return True

    def supportsUpdate(self):
        return True

    def importableByDefault(self):
        return True

    def importObject(self, prim, parent=None):
        self.actions.append('import ' + str(prim.GetPath()))
        return True

    def postImport(self, prim):
        self.actions.append('postImport ' + str(prim.GetPath()))
        return True

    def generateUniqueKey(self, prim):
        return hashlib.md5(str(prim.GetPath())).hexdigest()

    def update(self, prim):
        self.actions.append('update ' + str(prim.GetPath()))
        return True

    def preTearDown(self, prim):
        return True

    def tearDown(self, path):
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

    @unittest.skipIf(sys.version_info[0] >= 3, "RecursionError: maximum recursion depth exceeded while calling a Python object")
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

    @unittest.skipIf(sys.version_info[0] >= 3, "RecursionError: maximum recursion depth exceeded while calling a Python object")
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

    def test_variantSwitch_listener_from_different_stage(self):
        """Test listener only responds to changes made to layers found in proxy shape owned stages."""

        usdmaya.TranslatorBase.registerTranslator(CubeGenerator(), 'beast_rig')

        # Make a dummy stage that mimics prim path found in test data
        otherHandle = tempfile.NamedTemporaryFile(delete=True, suffix=".usda")
        otherHandle.close()

        # Scope
        if True:
            stage = Usd.Stage.CreateInMemory()
            stage.DefinePrim("/root/peter01")
            stage.Export(otherHandle.name)

        # Open both stages
        testStage = Usd.Stage.Open("../test_data/inactivetest.usda")
        otherStage = Usd.Stage.Open(otherHandle.name)

        # Cache
        stageCache = UsdUtils.StageCache.Get()
        stageCache.Insert(testStage)
        stageCache.Insert(otherStage)
        stageId = stageCache.GetId(testStage)

        # Import legit test data
        cmds.AL_usdmaya_ProxyShapeImport(stageId=stageId.ToLongInt(), name='bobo')

        # Make sure both paths are valid
        self.assertTrue(testStage.GetPrimAtPath("/root/peter01"))
        self.assertTrue(otherStage.GetPrimAtPath("/root/peter01"))

        # Modify stage that isn't loaded by AL_USDMaya
        prim = otherStage.GetPrimAtPath("/root/peter01")
        prim.SetActive(False)

        # Ensure stage on proxy wasn't modified
        self.assertEqual(CubeGenerator.getState()["tearDownCount"], 0)

        # Cleanup
        os.remove(otherHandle.name)

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

    def test_import_and_update_consistency(self):
        '''
        test consistency when called via TranslatePrim, or triggered via onObjectsChanged
        '''
        updateableTranslator = UpdateableTranslator()
        usdmaya.TranslatorBase.registerTranslator(updateableTranslator, 'test')

        stage = Usd.Stage.Open("../test_data/translator_update_postimport.usda")
        stageCache = UsdUtils.StageCache.Get()
        stageCache.Insert(stage)
        stageId = stageCache.GetId(stage)
        shapeName = 'updateProxyShape'
        cmds.AL_usdmaya_ProxyShapeImport(stageId=stageId.ToLongInt(), name=shapeName)

        # Verify if the methods have been called
        self.assertTrue("import /root/peter01/rig" in updateableTranslator.actions)
        self.assertTrue("postImport /root/peter01/rig" in updateableTranslator.actions)
        # "update()" method should not be called
        self.assertFalse("update /root/peter01/rig" in updateableTranslator.actions)

        updateableTranslator.actions = []
        cmds.AL_usdmaya_TranslatePrim(up="/root/peter01/rig", fi=True, proxy=shapeName)
        # "update()" should have been called
        self.assertTrue("update /root/peter01/rig" in updateableTranslator.actions)


class TestTranslatorUniqueKey(usdmaya.TranslatorBase):
    """
    Basic Translator for testing unique key
    """
    def __init__(self, *args, **kwargs):
        super(TestTranslatorUniqueKey, self).__init__(*args, **kwargs)
        self._supportsUpdate = False
        self.resetCounters()
        self.primHashValues = dict()

    def initialize(self):
        return True

    def resetCounters(self):
        self.preTearDownCount = 0
        self.tearDownCount = 0
        self.importObjectCount = 0
        self.postImportCount = 0
        self.updateCount = 0

    def setSupportsUpdate(self, state):
        self._supportsUpdate = state

    def generateUniqueKey(self, prim):
        return self.primHashValues.get(str(prim.GetPath()))

    def preTearDown(self, prim):
        self.preTearDownCount += 1
        return True

    def tearDown(self, path):
        self.tearDownCount += 1
        self.removeItems(path)
        return True

    def canExport(self, mayaObjectName):
        return False

    def needsTransformParent(self):
        return True

    def supportsUpdate(self):
        return self._supportsUpdate

    def importableByDefault(self):
        return True

    def exportObject(self, stage, path, usdPath, params):
        return

    def postImport(self, prim):
        self.postImportCount += 1
        return True

    def getTranslatedType(self):
        return Tf.Type.Unknown

    def importObject(self, prim, parent=None):
        self.importObjectCount += 1
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

        return True

    def update(self, prim):
        self.updateCount += 1
        return True


class TestPythonTranslatorsUniqueKey(unittest.TestCase):

    def setUp(self):
        cmds.file(force=True, new=True)
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)
        self.assertTrue(cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True))

        self.translator = TestTranslatorUniqueKey()

        usdmaya.TranslatorBase.registerTranslator(self.translator, 'beast_bindings')

        self.stage = Usd.Stage.Open('../test_data/rig_bindings.usda')
        self.rootPrim = self.stage.GetPrimAtPath('/root')

        self.stageCache = UsdUtils.StageCache.Get()
        self.stageCache.Insert(self.stage)
        self.stageId = self.stageCache.GetId(self.stage)

    def tearDown(self):
        UsdUtils.StageCache.Get().Clear()
        usdmaya.TranslatorBase.clearTranslators()
        self.translator = None
        self.stage = None
        self.stageCache = None
        self.stageId = None

    def test_withoutSupportsUpdate(self):
        self.translator.setSupportsUpdate(False)

        # pre-generate hash value for testing only
        self.translator.primHashValues['/root/static_four_cubes'] = 'static'
        self.translator.primHashValues['/root/dynamic_five_cubes'] = None

        vs = self.rootPrim.GetVariantSet('cubes')
        vs.SetVariantSelection('fourCubes')

        cmds.AL_usdmaya_ProxyShapeImport(stageId=self.stageId.ToLongInt(), name='bindings_grp')
        self.assertEqual(self.translator.preTearDownCount, 0)
        self.assertEqual(self.translator.tearDownCount, 0)
        self.assertEqual(self.translator.importObjectCount, 2)
        self.assertEqual(self.translator.postImportCount, 2)
        self.assertEqual(self.translator.updateCount, 0)
        self.assertTrue(cmds.objExists('|bindings_grp|root|static_four_cubes'))
        self.assertTrue(cmds.objExists('|bindings_grp|root|dynamic_five_cubes'))

        self.translator.resetCounters()
        # swapping variant set
        vs.SetVariantSelection('fiveCubes')

        # preTearDownCount happens two times:
        #  - one for before changing variant set
        #   "static_four_cubes", "dynamic_five_cubes", +2
        #  - one for before unloading when re-sync
        #   "dynamic_five_cubes", +1
        self.assertEqual(self.translator.preTearDownCount, 3)

        # "dynamic_five_cubes" has no hash, will be removed and recreated (+1)
        self.assertEqual(self.translator.tearDownCount, 1)
        self.assertEqual(self.translator.importObjectCount, 1)
        self.assertEqual(self.translator.postImportCount, 1)
        self.assertEqual(self.translator.updateCount, 0)
        self.assertTrue(cmds.objExists('|bindings_grp|root|static_four_cubes'))
        self.assertTrue(cmds.objExists('|bindings_grp|root|dynamic_five_cubes'))

        ###
        self.translator.resetCounters()

        # set hash to constant value for both
        # assuming something changed in Maya and invalided the hash
        self.translator.primHashValues['/root/static_four_cubes'] = 1242569910549196999 # <= hash(...)
        self.translator.primHashValues['/root/dynamic_five_cubes'] = -8694302015330580362
        # switch variant again
        vs.SetVariantSelection('fourCubes')

        # both of them will be removed and recreated
        self.assertEqual(self.translator.preTearDownCount, 4)
        self.assertEqual(self.translator.tearDownCount, 2)
        self.assertEqual(self.translator.importObjectCount, 2)
        self.assertEqual(self.translator.postImportCount, 2)
        self.assertEqual(self.translator.updateCount, 0)
        self.assertTrue(cmds.objExists('|bindings_grp|root|static_four_cubes'))
        self.assertTrue(cmds.objExists('|bindings_grp|root|dynamic_five_cubes'))

        ###
        self.translator.resetCounters()

        # switch variant without changing the hash, no update will happen
        vs.SetVariantSelection('fiveCubes')
        # two preTearDown() will be called to update the hash before unloading
        self.assertEqual(self.translator.preTearDownCount, 2)
        # no removing nor recreating will happen, the hash are the same
        self.assertEqual(self.translator.tearDownCount, 0)
        self.assertEqual(self.translator.importObjectCount, 0)
        self.assertEqual(self.translator.postImportCount, 0)
        self.assertEqual(self.translator.updateCount, 0)
        self.assertTrue(cmds.objExists('|bindings_grp|root|static_four_cubes'))
        self.assertTrue(cmds.objExists('|bindings_grp|root|dynamic_five_cubes'))

    def test_withSupportsUpdate(self):
        self.translator.setSupportsUpdate(True)

        # pre-generate hash value for testing only
        self.translator.primHashValues['/root/static_four_cubes'] = 1242569910549196999
        self.translator.primHashValues['/root/dynamic_five_cubes'] = None

        vs = self.rootPrim.GetVariantSet('cubes')
        vs.SetVariantSelection('fourCubes')

        cmds.AL_usdmaya_ProxyShapeImport(stageId=self.stageId.ToLongInt(), name='bindings_grp')
        self.assertEqual(self.translator.preTearDownCount, 0)
        self.assertEqual(self.translator.tearDownCount, 0)
        self.assertEqual(self.translator.importObjectCount, 2)
        self.assertEqual(self.translator.postImportCount, 2)
        self.assertEqual(self.translator.updateCount, 0)
        self.assertTrue(cmds.objExists('|bindings_grp|root|static_four_cubes'))
        self.assertTrue(cmds.objExists('|bindings_grp|root|dynamic_five_cubes'))

        ###
        self.translator.resetCounters()
        # swapping variant set
        # "static_four_cubes" will not be updated;
        # "dynamic_five_cubes" will be updated
        vs.SetVariantSelection('fiveCubes')

        # preTearDownCount happens two times:
        #  - one for before changing variant set
        #   "static_four_cubes", "dynamic_five_cubes", +2
        #  - one for before unloading when re-sync
        #    (none for this case) +0
        self.assertEqual(self.translator.preTearDownCount, 2)

        # "dynamic_five_cubes" has no hash, will be updated (+1)
        self.assertEqual(self.translator.tearDownCount, 0)
        self.assertEqual(self.translator.importObjectCount, 0)
        self.assertEqual(self.translator.postImportCount, 1)
        self.assertEqual(self.translator.updateCount, 1)
        self.assertTrue(cmds.objExists('|bindings_grp|root|static_four_cubes'))
        self.assertTrue(cmds.objExists('|bindings_grp|root|dynamic_five_cubes'))

        ###
        self.translator.resetCounters()

        # set hash to constant value for both
        # assuming something changed in Maya and invalided the hash
        self.translator.primHashValues['/root/static_four_cubes'] = '12345'
        self.translator.primHashValues['/root/dynamic_five_cubes'] = '12345'
        # switch variant again
        # both of them will be updated
        vs.SetVariantSelection('fourCubes')

        # both of them will be removed and recreated
        self.assertEqual(self.translator.preTearDownCount, 2)
        self.assertEqual(self.translator.tearDownCount, 0)
        self.assertEqual(self.translator.importObjectCount, 0)
        self.assertEqual(self.translator.postImportCount, 2)
        self.assertEqual(self.translator.updateCount, 2)
        self.assertTrue(cmds.objExists('|bindings_grp|root|static_four_cubes'))
        self.assertTrue(cmds.objExists('|bindings_grp|root|dynamic_five_cubes'))

        ###
        self.translator.resetCounters()

        # switch variant without changing the hash, no update will happen
        vs.SetVariantSelection('fiveCubes')
        # two preTearDown() will be called to update the hash before unloading
        self.assertEqual(self.translator.preTearDownCount, 2)
        # no removing nor recreating will happen, the hash are the same
        self.assertEqual(self.translator.tearDownCount, 0)
        self.assertEqual(self.translator.importObjectCount, 0)
        self.assertEqual(self.translator.postImportCount, 0)
        self.assertEqual(self.translator.updateCount, 0)
        self.assertTrue(cmds.objExists('|bindings_grp|root|static_four_cubes'))
        self.assertTrue(cmds.objExists('|bindings_grp|root|dynamic_five_cubes'))


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
