#!/usr/bin/env mayapy
#
# Copyright 2019 Autodesk
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

from pxr import Usd
from pxr import UsdGeom

from maya import cmds
from maya import standalone

import fixturesUtils

import os
import unittest


class testUsdExportMayaInstancer(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)

        cls._clsTestName = 'UsdExportMayaInstancerTest'

        cls.mayaFile = os.path.join(inputPath, cls._clsTestName,
            "%s.ma" % cls._clsTestName)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()
        
    def doExport(self, **kwargs):
        # Export to USD.
        usdFilePath = os.path.abspath('%s_%s.usda' % (self._clsTestName,
                                                      self._testMethodName))
        
        finalKwargs = {
            'mergeTransformAndShape': True,
            'file': usdFilePath,
            'frameRange': [1, 5],
            'shadingMode': 'none',
        }
        finalKwargs.update(kwargs)
        cmds.usdExport(**finalKwargs)

        return Usd.Stage.Open(usdFilePath)
    
    def doTest(self, namespacedInstancer, namespacedPrototype, stripNamespaces):
        cmds.file(self.mayaFile, open=True, force=True)
        group = 'group1'
        namespace = 'mayaNamespace'
        instancer = baseInstancer = 'instancer1'
        cubeTrans = baseCubeTrans = 'pCube1'
        cubeShape = baseCubeShape = 'pCubeShape1'
        
        # Note: the scene already has the namespace defined, so we don't
        # need to do that
        if namespacedInstancer:
            instancer = '{}:{}'.format(namespace, baseInstancer)
            cmds.rename(baseInstancer, instancer)
            self.assertFalse(cmds.objExists(baseInstancer))
        self.assertTrue(cmds.objExists(instancer))
        
        if namespacedPrototype:
            cubeTrans = '{}:{}'.format(namespace, baseCubeTrans)
            cubeShape = '{}:{}'.format(namespace, baseCubeShape)
            # renaming trans should also rename shape
            cmds.rename(baseCubeTrans, cubeTrans)
            self.assertFalse(cmds.objExists(baseCubeTrans))
            self.assertFalse(cmds.objExists(baseCubeShape))
        self.assertTrue(cmds.objExists(cubeTrans))
        self.assertTrue(cmds.objExists(cubeShape))
        
        stage = self.doExport(stripNamespaces=stripNamespaces)
        self.assertTrue(stage)
        
        if namespacedInstancer and not stripNamespaces:
            instancerPrimPath = '/{}/{}_{}'.format(group, namespace, baseInstancer)
        else:
            instancerPrimPath = '/{}/{}'.format(group, baseInstancer)
            
        instancerPrim = stage.GetPrimAtPath(instancerPrimPath)
        self.assertTrue(instancerPrim)
        instancer = UsdGeom.PointInstancer(instancerPrim)
        self.assertTrue(instancer)
        
        if namespacedPrototype and not stripNamespaces:
            protoPrimPath = '{}/Prototypes/{}_{}_0'.format(
                instancerPrimPath, namespace, baseCubeTrans)
        else:
            protoPrimPath = '{}/Prototypes/{}_0'.format(
                instancerPrimPath, baseCubeTrans)
            
        protoPrim = stage.GetPrimAtPath(protoPrimPath)
        self.assertTrue(protoPrim)
        protoMesh = UsdGeom.Mesh(protoPrim)
        self.assertTrue(protoMesh)
        
        prototypes = instancer.GetPrototypesRel().GetTargets()
        self.assertEqual(prototypes, [protoPrim.GetPath()])
        

def addExportInstancerTest(namespacedInstancer, namespacedPrototype,
                           stripNamespaces):
    def testMethod(self):
        self.doTest(namespacedInstancer, namespacedPrototype, stripNamespaces)
    testName = 'testExportInstancer'
    conditions = []
    if namespacedInstancer:
        testName += '_nsInstancer'
    if namespacedPrototype:
        testName += '_nsProto'
    if stripNamespaces:
        testName += '_stripNS'
    testMethod.__name__ = testName
    
    doc = 'Tests that a Maya instancer exports as a UsdGeomPointInstancer '\
        'schema prim'
    conditions = []
    if namespacedInstancer:
        conditions.append('instancer is namespaced')
    if namespacedPrototype:
        conditions.append('prototype is namespaced')
    if stripNamespaces:
        conditions.append('stripNamespaces is enabled')
    if len(conditions) > 1:
        conditions[-2:] = ['{} and {}'.format(conditions[-2], conditions[-1])]
    if conditions:
        doc += ' if ' + ', '.join(conditions)
    doc += '.'
    testMethod.__doc__ = doc
    
    setattr(testUsdExportMayaInstancer, testName, testMethod)

for namespacedInstancer in (False, True):
    for namespacedPrototype in (False, True):
        for stripNamespaces in (False, True):
            addExportInstancerTest(namespacedInstancer, namespacedPrototype,
                                   stripNamespaces)


if __name__ == '__main__':
    unittest.main(verbosity=2)
