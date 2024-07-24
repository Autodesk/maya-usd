#!/usr/bin/env mayapy
#
# Copyright 2021 Luma
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
import unittest

from maya import cmds
from maya import standalone

from pxr import Usd
from pxr import UsdGeom
from pxr import Vt
from pxr import Gf

import fixturesUtils

class testUsdExportRoot(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.setUpClass(__file__)
        cls.filePath = os.path.join(inputPath, "UsdExportRootsTest", "UsdExportRootsTest.ma")

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()
        
    def setUp(self):
        cmds.file(testUsdExportRoot.filePath, force=True, open=True)

    def doExportImportOneMethod(self, method, shouldError=False, root=None, selection=None, worldspace=False):
        if isinstance(root, str):
            root = [root]
        if isinstance(selection, str):
            selection = [selection]
            
        name = 'UsdExportRoot_root-{root}_sel-{sel}'.format(root='-'.join(root or []), sel='-'.join(selection or []))
        if method == 'cmd':
            name += '.usda'
        elif method == 'translator':
            # the translator seems to be only able to export .usd - or at least,
            # I couldn't find an option to do .usda instead...
            name += '.usd'
        usdFile = os.path.abspath(name)
        
        if method == 'cmd':
            exportMethod = cmds.mayaUSDExport
            args = ()
            kwargs = {
                'file': usdFile,
                'mergeTransformAndShape': True,
                'shadingMode': 'useRegistry',
                'legacyMaterialScope': False,
                'defaultPrim': 'None',
            }
            if root:
                kwargs['exportRoots'] = root
            if selection:
                kwargs['selection'] = 1
                cmds.select(selection)
            if worldspace:
                kwargs['worldspace'] = 1
        else:
            exportMethod = cmds.file
            args = (usdFile,)
            kwargs = {
                'type': 'USD Export',
                'f': 1,
            }
            options = ['legacyMaterialScope=0', 'defaultPrim=None']
            if root:
                options.append('exportRoots={}'.format(','.join(root)))
            if worldspace:
                options.append('worldspace=%d' % worldspace)
            if selection:
                kwargs['exportSelected'] = 1
                cmds.select(selection)
            else:
                kwargs['exportAll'] = 1
            if options:
                kwargs['options'] = ';'.join(options)

        if shouldError:
            self.assertRaises(RuntimeError, exportMethod, *args, **kwargs)
            return
        exportMethod(*args, **kwargs)
        return Usd.Stage.Open(usdFile)
        
    def doExportImportTest(self, validator, shouldError=False, root=None, selection=None, worldspace=False):
        stage = self.doExportImportOneMethod('cmd', shouldError=shouldError,
                                             root=root, selection=selection, worldspace=worldspace)
        validator(stage)
        stage = self.doExportImportOneMethod('translator', shouldError=shouldError,
                                             root=root, selection=selection, worldspace=worldspace)
        validator(stage)

    def assertPrim(self, stage, path, type):
        prim = stage.GetPrimAtPath(path)
        self.assertTrue(prim.IsValid(), "Expected to find %s" % path)
        self.assertEqual(prim.GetTypeName(), type, "Expected prim %s to have type %s" % (path, type))

    def assertPrimXform(self, stage, path, xforms):
        '''
        Verify that the prim has the given xform in the roder given.
        xforms should be a list of pairs, each containing the xform op name and its value.
        '''
        prim = stage.GetPrimAtPath(path)
        xformOpOrder = prim.GetAttribute('xformOpOrder').Get()
        self.assertEqual(len(xformOpOrder), len(xforms))
        for name, value in xforms:
            self.assertEqual(xformOpOrder[0], name)
            attr = prim.GetAttribute(name)
            self.assertIsNotNone(attr)
            self.assertEqual(attr.Get(), value)
            # Chop off the first xofrm op for the next loop.
            xformOpOrder = xformOpOrder[1:]

    def assertNotPrim(self, stage, path):
        self.assertFalse(stage.GetPrimAtPath(path).IsValid(), "Did not expect to find %s" % path)

    def testNonOverlappingSelectionRoot(self):
        # test that the command errors if we give it a root + selection that
        # have no intersection
        def validator(stage):
            pass
        self.doExportImportTest(validator, shouldError=True, root='Mid', selection='OtherTop')

    def testExportRoot_rootTop_selNone(self):
        def validator(stage):
            self.assertPrim(stage, '/Top/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertPrim(stage, '/Top/OtherMid', 'Xform')
            self.assertPrim(stage, '/Top/Mid/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Top', [
                ('xformOp:translate', (2., 0., 0.)),
                ('xformOp:rotateXYZ', (45., 0., 0.))])
            self.assertPrimXform(stage, '/Top/Mid', [
                ('xformOp:translate', (0., 2., 0.)),
                ('xformOp:rotateXYZ', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Top/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Top')

    def testExportRoot_rootTop_selNone_worldspace(self):
        def validator(stage):
            self.assertPrim(stage, '/Top/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertPrim(stage, '/Top/OtherMid', 'Xform')
            self.assertPrim(stage, '/Top/Mid/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Top', [
                ('xformOp:translate', (2., 0., 0.)),
                ('xformOp:rotateXYZ', (45., 0., 0.))])
            self.assertPrimXform(stage, '/Top/Mid', [
                ('xformOp:translate', (0., 2., 0.)),
                ('xformOp:rotateXYZ', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Top/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Top', worldspace=True)

    def testExportRoot_rootTop_selTop(self):
        def validator(stage):
            self.assertPrim(stage, '/Top/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertPrim(stage, '/Top/OtherMid', 'Xform')
            self.assertPrim(stage, '/Top/Mid/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Top', [
                ('xformOp:translate', (2., 0., 0.)),
                ('xformOp:rotateXYZ', (45., 0., 0.))])
            self.assertPrimXform(stage, '/Top/Mid', [
                ('xformOp:translate', (0., 2., 0.)),
                ('xformOp:rotateXYZ', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Top/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Top', selection='Top')

    def testExportRoot_rootTop_selMid(self):
        def validator(stage):
            self.assertPrim(stage, '/Top/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/Top/OtherMid')
            self.assertPrim(stage, '/Top/Mid/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Top', [
                ('xformOp:translate', (2., 0., 0.)),
                ('xformOp:rotateXYZ', (45., 0., 0.))])
            self.assertPrimXform(stage, '/Top/Mid', [
                ('xformOp:translate', (0., 2., 0.)),
                ('xformOp:rotateXYZ', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Top/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Top', selection='Mid')

    def testExportRoot_rootTop_selCube(self):
        def validator(stage):
            self.assertPrim(stage, '/Top/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/Top/OtherMid')
            self.assertNotPrim(stage, '/Top/Mid/OtherLowest')
            self.assertPrimXform(stage, '/Top', [
                ('xformOp:translate', (2., 0., 0.)),
                ('xformOp:rotateXYZ', (45., 0., 0.))])
            self.assertPrimXform(stage, '/Top/Mid', [
                ('xformOp:translate', (0., 2., 0.)),
                ('xformOp:rotateXYZ', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Top/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Top', selection='Cube')

    def testExportRoot_rootMid_selNone(self):
        def validator(stage):
            self.assertPrim(stage, '/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertPrim(stage, '/Mid/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Mid', [
                ('xformOp:translate', (0., 2., 0.)),
                ('xformOp:rotateXYZ', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Mid')

    def testExportRoot_rootMid_selNone_worldspace(self):
        def validator(stage):
            self.assertPrim(stage, '/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertPrim(stage, '/Mid/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Mid', [
                ('xformOp:translate', (2., 0., 0.)),
                ('xformOp:rotateXYZ', (45., 0., 0.)),
                ('xformOp:translate:channel1', (0., 2., 0.)),
                ('xformOp:rotateXYZ:channel1', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Mid', worldspace=True)

    def testExportRoot_rootMid_selTop(self):
        def validator(stage):
            self.assertPrim(stage, '/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertPrim(stage, '/Mid/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Mid', [
                ('xformOp:translate', (0., 2., 0.)),
                ('xformOp:rotateXYZ', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Mid', selection='Top')

    def testExportRoot_rootMid_selMid(self):
        def validator(stage):
            self.assertPrim(stage, '/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertPrim(stage, '/Mid/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Mid', [
                ('xformOp:translate', (0., 2., 0.)),
                ('xformOp:rotateXYZ', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Mid', selection='Mid')

    def testExportRoot_rootMid_selCube(self):
        def validator(stage):
            self.assertPrim(stage, '/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/Mid/OtherLowest')
            self.assertPrimXform(stage, '/Mid', [
                ('xformOp:translate', (0., 2., 0.)),
                ('xformOp:rotateXYZ', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Mid', selection='Cube')

    def testExportRoot_rootCube_selNone(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
            self.assertPrimXform(stage, '/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Cube')

    def testExportRoot_rootCube_selNone_worldspace(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
            self.assertPrimXform(stage, '/Cube', [
                ('xformOp:translate', (2., 0., 0.)),
                ('xformOp:rotateXYZ', (45., 0., 0.)),
                ('xformOp:translate:channel1', (0., 2., 0.)),
                ('xformOp:rotateXYZ:channel1', (0., 0., 45.)),
                ('xformOp:translate:channel2', (0., 0., 3.)),
                ('xformOp:rotateXYZ:channel2', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Cube', worldspace=True)

    def testExportRoot_rootCube_selTop(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
            self.assertPrimXform(stage, '/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Cube', selection='Top')

    def testExportRoot_rootCube_selMid(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
            self.assertPrimXform(stage, '/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Cube', selection='Mid')

    def testExportRoot_rootCube_selCube(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
            self.assertPrimXform(stage, '/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='Cube', selection='Cube')
        
    def testExportRoot_rootEMPTY_selCube(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
            self.assertPrimXform(stage, '/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='', selection='Cube')

    def testExportRoot_rootEMPTY_selCubeOtherLowest(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertPrim(stage, '/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root='', selection=['Cube','OtherLowest'])
        
    def testExportRoot_rootCubeOtherLowest_selNone(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertPrim(stage, '/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root=['Cube','OtherLowest'])

    def testExportRoot_rootCubeOtherLowest_selCube(self):
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/Mid')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
            self.assertPrimXform(stage, '/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root=['Cube','OtherLowest'], selection='Cube')

    def testExportRoot_rootCubeOtherLowest_selCubeOtherLowest(self):
        def validator(stage):
            self.assertPrim(stage, '/Mid/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertPrim(stage, '/OtherLowest', 'Xform')
            self.assertPrimXform(stage, '/Mid', [
                ('xformOp:translate', (0., 2., 0.)),
                ('xformOp:rotateXYZ', (0., 0., 45.))])
            self.assertPrimXform(stage, '/Mid/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root=['Mid','OtherLowest'], selection=['Cube','OtherLowest'])

    def testExportRoot_rootCubeCube1_selCubeCube1(self):
        # test that two objects from sibling hierarchies export correctly with materials
        def validator(stage):
            self.assertPrim(stage, '/Cube', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert2SG/lambert2', 'Shader')
            self.assertNotPrim(stage, '/Top')
            self.assertNotPrim(stage, '/OtherTop')
            self.assertNotPrim(stage, '/OtherMid')
            self.assertNotPrim(stage, '/OtherLowest')
            self.assertPrim(stage, '/Cube1', 'Mesh')
            self.assertPrim(stage, '/Looks/lambert3SG/lambert3', 'Shader')
            self.assertPrimXform(stage, '/Cube', [
                ('xformOp:translate', (0., 0., 3.)),
                ('xformOp:rotateXYZ', (0., 45., 0.))])
        self.doExportImportTest(validator, root=['Cube','Cube1'], selection=['Cube','Cube1'])

    def testDagPathValidation(self):
        # test that the command errors if we give it an invalid dag path
        def validator(stage):
            pass
        self.doExportImportTest(validator, shouldError=True, root='NoneExisting', selection='OtherTop')

    def testExportRootStripNamespace(self):
        # Test that stripnamespace works with export roots when stripped node names would conflict
        # but only one was specified as root.

        cmds.file(new=True, force=True)

        cmds.namespace(add="myAssetA")
        cmds.polySphere(name="myAssetA:sphere_GEO")
        cmds.namespace(add="myAssetB")
        cmds.polySphere(name="myAssetB:sphere_GEO")

        usdFile = os.path.abspath("testExportRootStripNamespace.usda")

        cmds.mayaUSDExport(file=usdFile,
            exportRoots=['|myAssetA:sphere_GEO'],
            stripNamespaces=True,
            )
            
        stage = Usd.Stage.Open(usdFile)

        self.assertPrim(stage, '/sphere_GEO', 'Mesh')


if __name__ == '__main__':
    unittest.main(verbosity=2)
