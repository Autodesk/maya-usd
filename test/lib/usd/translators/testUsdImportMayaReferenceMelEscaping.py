#!/usr/bin/env mayapy
#
# Copyright 2026 Autodesk
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

from pxr import Sdf, Usd

from maya import cmds
from maya import standalone

import os
import unittest

import fixturesUtils

class testUsdImportMayaReferenceMelEscaping(unittest.TestCase):
    '''
    Strings authored on Maya reference prims end up inside MEL commands. They
    must stay string values and never be run as MEL.
    '''

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)
        cls.mayaFile = os.path.join(
            inputPath, "UsdImportMayaReferenceTest",
            "UsdExportSkeletonWithoutBindPose.ma").replace('\\', '/')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def _importReferencePrim(self, fileName, namespace, mayaFile, referenceNodeName=None):
        stage = Usd.Stage.CreateNew(os.path.abspath(fileName))
        stage.SetDefaultPrim(stage.DefinePrim('/World', 'Xform'))
        prim = stage.DefinePrim('/World/Rig', 'MayaReference')
        prim.CreateAttribute('mayaNamespace', Sdf.ValueTypeNames.String).Set(namespace)
        prim.CreateAttribute('mayaReference', Sdf.ValueTypeNames.Asset).Set(mayaFile)
        if referenceNodeName:
            prim.CreateAttribute('MayaReferenceNodeName', Sdf.ValueTypeNames.String).Set(
                referenceNodeName)
        stage.GetRootLayer().Save()
        cmds.usdImport(file=stage.GetRootLayer().realPath, shadingMode=[['none', 'default'], ])

    def _injection(self, nodeName):
        # Ends with a command that returns nothing: the import treats the last
        # command's result as the created reference node.
        return '" "%s"; createNode transform -n %s; select -clear; //' % (
            self.mayaFile, nodeName)

    def testNamespaceIsNotRunAsMel(self):
        self._importReferencePrim(
            'namespaceInjection.usda', 'rig' + self._injection('injectedByNamespace'),
            self.mayaFile)

        self.assertFalse(cmds.objExists('injectedByNamespace'))

    def testReferencePathIsNotRunAsMel(self):
        # Maya cuts file paths at ';', so a path can only inject MEL through a
        # backquoted command used as an extra argument.
        self._importReferencePrim(
            'pathInjection.usda', 'rig',
            self.mayaFile + '" `createNode transform -n injectedByPath` "')

        self.assertFalse(cmds.objExists('injectedByPath'))

    def testNamespaceChangeIsNotRunAsMel(self):
        self._importReferencePrim('namespaceBefore.usda', 'rig', self.mayaFile)
        referenceNodes = [node for node in cmds.ls(type='reference')
                          if node != 'sharedReferenceNode']
        self.assertEqual(len(referenceNodes), 1)

        # Naming the existing reference node makes the import reuse it and only
        # change its namespace.
        self._importReferencePrim(
            'namespaceAfter.usda', 'rig2' + self._injection('injectedByNamespaceChange'),
            self.mayaFile, referenceNodes[0])

        self.assertFalse(cmds.objExists('injectedByNamespaceChange'))

if __name__ == '__main__':
    unittest.main(verbosity=2)
