#!/usr/bin/env mayapy
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

import mayaUsd.lib as mayaUsdLib
import mayaUsd.ufe as mayaUsdUfe

from pxr import Plug, Usd, Sdf, Tf

from maya import cmds
from maya.api import OpenMaya
from maya import standalone

import fixturesUtils

import unittest

_customRigTypeName = None

class customRigPrimReader(mayaUsdLib.PrimReader):
    def Read(self, context):
        usdPrim = self._GetArgs().GetUsdPrim()
        rigNode = cmds.spaceLocator(name=usdPrim.GetName())
        
        numCubesAttr = usdPrim.GetAttribute("cubes")
        numCubes = numCubesAttr.Get()
        
        for x in range(numCubes):
            nodes = cmds.polyCube()
            cmds.parent(nodes[0], rigNode)
        
        parent = context.GetMayaNode(usdPrim.GetPath().GetParentPath(), True)
        parentDagPath = OpenMaya.MFnDagNode(parent).fullPathName()
        cmds.parent(rigNode, parentDagPath)

        selectionList = OpenMaya.MSelectionList()
        selectionList.add(rigNode[0])
        rigNodeObj = selectionList.getDependNode(0)
        # Add USD original type information to corresponding Maya node.
        mayaUsdLib.TranslatorUtil.SetUsdTypeName(rigNodeObj, _customRigTypeName)
        nodePath = usdPrim.GetPath().pathString
        context.RegisterNewMayaNode(nodePath, rigNodeObj);

        return True

class customRigPrimUpdater(mayaUsdLib.PrimUpdater):
    def __init__(self, *args, **kwargs):
        super(customRigPrimUpdater, self).__init__(*args, **kwargs)

    def shouldAutoEdit(self):
        autoEditAttr = self.getUsdPrim().GetAttribute("autoEdit")
        if autoEditAttr == None:
            return False
            
        return autoEditAttr.Get()

    def editAsMaya(self):
        return super(customRigPrimUpdater, self).editAsMaya()

    def discardEdits(self):
        return super(customRigPrimUpdater, self).discardEdits()

    def pushCopySpecs(self, srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath):
        newPrimDstSdfPath = "{}_cache".format(dstSdfPath)
        Sdf.CopySpec(srcLayer,srcSdfPath,dstLayer,newPrimDstSdfPath)
        return customRigPrimUpdater.PushCopySpecs.Prune

    def pushEnd(self):
        return super(customRigPrimUpdater, self).pushEnd()

class testCustomRig(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)
        
        global _customRigTypeName
        _customRigTypeName = Usd.SchemaRegistry.GetTypeFromSchemaTypeName("CustomRig").typeName

        mayaUsdLib.PrimReader.Register(customRigPrimReader, _customRigTypeName)
        mayaUsdLib.PrimUpdater.Register(customRigPrimUpdater, _customRigTypeName, "transform", customRigPrimUpdater.Supports.All.value + customRigPrimUpdater.Supports.AutoPull.value)


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def _GetMFnDagNode(self, objectName):
         selectionList = OpenMaya.MSelectionList()
         try:
            selectionList.add(objectName)
         except:
            return None
            
         mObj = selectionList.getDependNode(0)

         return OpenMaya.MFnDagNode(mObj)

    def testCustomRigSchema(self):
        "Validate that codeless schema was discovered and loaded successfully"
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition("CustomRig")
        self.assertTrue(primDef)
        
        type = Tf.Type.FindByName("MayaUsdSchemaTestRig")
        self.assertEqual( type, Usd.SchemaRegistry.GetTypeFromSchemaTypeName("CustomRig") )
        
        self.assertEqual(primDef.GetPropertyNames(), ["cubes"])

        layer = Sdf.Layer.CreateAnonymous(".usd")
        stage = Usd.Stage.Open(layer, Usd.Stage.LoadAll)
        typedPrim = stage.DefinePrim("/bob", "CustomRig")
        self.assertTrue(typedPrim)

    def testCustomRigReader(self):
        "Validate prim reader for CustomRig codeless schema"
        
        layer = Sdf.Layer.CreateAnonymous(".usd")
        layer.ImportFromString(
        ''' #usda 1.0
            (
                defaultPrim = "world"
            )
            def Xform "world" {
                def Xform "anim" {
                    def CustomRig "bob" {
                        int cubes = 2
                    }
                }
            }
        '''
        )

        cmds.usdImport(file=layer.identifier)
        
        self.assertTrue(self._GetMFnDagNode("world|anim|bob"))
        self.assertTrue(self._GetMFnDagNode("world|anim|bob|pCube1"))
        self.assertTrue(self._GetMFnDagNode("world|anim|bob|pCube2"))
        self.assertFalse(self._GetMFnDagNode("world|anim|bob|pCube3"))
       
    def testCustomRigUpdater(self):
        "Validate prim updater for CustomRig codeless schema"
        
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        
        stage = mayaUsdUfe.getStage(proxyShape)
        self.assertTrue(stage)
        
        layer = stage.GetRootLayer()
        layer.ImportFromString(
        ''' #sdf 1
            (
                defaultPrim = "world"
            )
            def Xform "world" {
                def Xform "anim" {
                    def CustomRig "bob" {
                        int cubes = 2
                    }
                }
            }
        '''
        )
        
        bobUfePathStr = "{},/world/anim/bob".format(proxyShape)
        
        # Pull the object for editing in Maya
        self.assertTrue(mayaUsdLib.PrimUpdaterManager.editAsMaya(bobUfePathStr))
        
        # Retrieve pulled object
        bobMayaPathStr = cmds.ls(sl=True)[0]
        
        # Test partial paths to make the test more robust with any changes around "invisible" __mayaUsd__
        # parent
        self.assertTrue(self._GetMFnDagNode("bob|pCube1"))
        self.assertTrue(self._GetMFnDagNode("bob|pCube2"))
        
        # Animate the cubes
        cmds.playbackOptions( minTime=0, maxTime=10 )
        cmds.setKeyframe( "bob|pCube1.ry", time=1.0, value=0 )
        cmds.setKeyframe( "bob|pCube1.ry", time=10.0, value=100 )
        cmds.setKeyframe( "bob|pCube2.ty", time=1.0, value=0 )
        cmds.setKeyframe( "bob|pCube2.ty", time=10.0, value=10 )
        
        # Push the animation back to USD. This will use a custom logic that will write it to a new prim
        self.assertTrue(mayaUsdLib.PrimUpdaterManager.mergeToUsd(bobMayaPathStr))
        
        # After push, all Maya objects should be gone
        self.assertFalse(self._GetMFnDagNode("bob"))
        self.assertFalse(self._GetMFnDagNode("bob|pCube1"))
        self.assertFalse(self._GetMFnDagNode("bob|pCube2"))
        
        # Validate the data we pushed back via custom updater
        self.assertTrue(stage.GetPrimAtPath("/world/anim/bob_cache"))
        self.assertTrue(stage.GetPrimAtPath("/world/anim/bob_cache/pCube1"))
        self.assertTrue(stage.GetPrimAtPath("/world/anim/bob_cache/pCube2"))
        
        prim = stage.GetPrimAtPath("/world/anim/bob_cache/pCube2")
        attr = prim.GetAttribute("xformOp:translate")
        self.assertEqual(attr.GetNumTimeSamples(), 11)
        self.assertEqual(attr.GetTimeSamples(), [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0])

    def testCustomRigUpdaterSubsetAnim(self):
        "Validate prim updater for CustomRig codeless schema but with only a subset of frames"
        
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        
        stage = mayaUsdUfe.getStage(proxyShape)
        self.assertTrue(stage)
        
        layer = stage.GetRootLayer()
        layer.ImportFromString(
        ''' #sdf 1
            (
                defaultPrim = "world"
            )
            def Xform "world" {
                def Xform "anim" {
                    def CustomRig "bob" {
                        int cubes = 2
                    }
                }
            }
        '''
        )
        
        bobUfePathStr = "{},/world/anim/bob".format(proxyShape)
        
        # Pull the object for editing in Maya
        self.assertTrue(mayaUsdLib.PrimUpdaterManager.editAsMaya(bobUfePathStr))
        
        # Retrieve pulled object
        bobMayaPathStr = cmds.ls(sl=True)[0]
        
        # Test partial paths to make the test more robust with any changes around "invisible" __mayaUsd__
        # parent
        self.assertTrue(self._GetMFnDagNode("bob|pCube1"))
        self.assertTrue(self._GetMFnDagNode("bob|pCube2"))
        
        # Animate the cubes
        cmds.playbackOptions( minTime=0, maxTime=10 )
        cmds.setKeyframe( "bob|pCube1.ry", time=1.0, value=0 )
        cmds.setKeyframe( "bob|pCube1.ry", time=10.0, value=100 )
        cmds.setKeyframe( "bob|pCube2.ty", time=1.0, value=0 )
        cmds.setKeyframe( "bob|pCube2.ty", time=10.0, value=10 )
        
        # Push the animation back to USD. This will use a custom logic that will write it to a new prim
        userArgs = {
            'animation': True,
            'startTime': 0.,
            'endTime': 9.0,
            'frameStride': 2.0,
        }
        self.assertTrue(mayaUsdLib.PrimUpdaterManager.mergeToUsd(bobMayaPathStr, userArgs))
        
        # After push, all Maya objects should be gone
        self.assertFalse(self._GetMFnDagNode("bob"))
        self.assertFalse(self._GetMFnDagNode("bob|pCube1"))
        self.assertFalse(self._GetMFnDagNode("bob|pCube2"))
        
        # Validate the data we pushed back via custom updater
        self.assertTrue(stage.GetPrimAtPath("/world/anim/bob_cache"))
        self.assertTrue(stage.GetPrimAtPath("/world/anim/bob_cache/pCube1"))
        self.assertTrue(stage.GetPrimAtPath("/world/anim/bob_cache/pCube2"))
        
        prim = stage.GetPrimAtPath("/world/anim/bob_cache/pCube2")
        attr = prim.GetAttribute("xformOp:translate")
        self.assertEqual(attr.GetNumTimeSamples(), 5)
        self.assertEqual(attr.GetTimeSamples(), [0.0, 2.0, 4.0, 6.0, 8.0])

    def testCustomRigUpdaterAutoEditLoad(self):
        "Validate auto edit on stage load for CustomRig codeless schema"
        
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        
        stage = mayaUsdUfe.getStage(proxyShape)
        self.assertTrue(stage)
        
        layer = stage.GetRootLayer()
        layer.ImportFromString(
        ''' #sdf 1
            (
                defaultPrim = "world"
            )
            def Xform "world" {
                def Xform "anim" {
                    def CustomRig "bob" {
                        int cubes = 2
                        bool autoEdit = 1
                    }
                }
            }
        '''
        )

        self.assertTrue(self._GetMFnDagNode("bob|pCube1"))
        self.assertTrue(self._GetMFnDagNode("bob|pCube2"))
        
        bobPrim = stage.GetPrimAtPath("/world/anim/bob")
        autoEditAttr = bobPrim.GetAttribute("autoEdit")
        self.assertTrue(autoEditAttr.Get())
        
    def testCustomRigUpdaterAutoEditChange(self):
        "Validate auto edit on attr change for CustomRig codeless schema"
        
        import mayaUsd_createStageWithNewLayer
        proxyShape = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        
        stage = mayaUsdUfe.getStage(proxyShape)
        self.assertTrue(stage)
        
        layer = stage.GetRootLayer()
        layer.ImportFromString(
        ''' #sdf 1
            (
                defaultPrim = "world"
            )
            def Xform "world" {
                def Xform "anim" {
                    def CustomRig "bob" {
                        int cubes = 2
                        bool autoEdit = 0
                    }
                }
            }
        '''
        )

        self.assertFalse(self._GetMFnDagNode("bob|pCube1"))
        self.assertFalse(self._GetMFnDagNode("bob|pCube2"))
        
        bobPrim = stage.GetPrimAtPath("/world/anim/bob")
        autoEditAttr = bobPrim.GetAttribute("autoEdit")
        self.assertFalse(autoEditAttr.Get())
        
        autoEditAttr.Set(True)
        self.assertTrue(autoEditAttr.Get())
 
        self.assertTrue(self._GetMFnDagNode("bob|pCube1"))
        self.assertTrue(self._GetMFnDagNode("bob|pCube2"))

if __name__ == '__main__':
    unittest.main(verbosity=2)
