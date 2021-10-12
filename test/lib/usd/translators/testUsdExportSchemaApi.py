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

import os
import unittest

from maya import cmds
from maya import standalone
import maya.api.OpenMaya as om

from pxr import Tf, Gf, UsdMaya, Usd

import fixturesUtils

HAS_BULLET = False
try:
    import maya.app.mayabullet.BulletUtils as BulletUtils
    import maya.app.mayabullet.RigidBody as RigidBody
    HAS_BULLET = True
except ImportError as ie:
    pass

class testUsdExportSchemaApi(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportSchemaApi(self):
        """Testing that a custom Schema API exporter is created and called"""
        mark = Tf.Error.Mark()
        mark.SetMark()
        self.assertTrue(mark.IsClean())

        cmds.polySphere(r=1)
        usdFilePath = os.path.abspath('UsdExportSchemaApiTest.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
                           extraContext=["NullAPI", "Moe"])

        self.assertFalse(mark.IsClean())

        errors = mark.GetErrors()
        messages = set()
        for e in errors:
            messages.add(e.commentary)
        expected = set([
            "Missing implementation for NullAPIChaser::ExportDefault",
            "Missing implementation for TestSchemaExporter::Write",
            "Missing implementation for TestSchemaExporter::PostExport"])
        self.assertEqual(messages, expected)

        cmds.file(f=True, new=True)

    def testExportSchemaApiViaFileAPI(self):
        """Testing that a custom Schema API exporter is created and called with file command"""
        mark = Tf.Error.Mark()
        mark.SetMark()
        self.assertTrue(mark.IsClean())

        cmds.polySphere(r=1)
        usdFilePath = os.path.abspath('UsdExportSchemaApiTestFile.usda')
        cmds.file(usdFilePath, force=True,
                  options="mergeTransformAndShape=1;extraContext=Thierry,NullAPI,Moe",
                  typ="USD Export", pr=True, ea=True)

        self.assertFalse(mark.IsClean())

        errors = mark.GetErrors()
        messages = set()
        for e in errors:
            messages.add(e.commentary)
        expected = set([
            "Missing implementation for NullAPIChaser::ExportDefault",
            "Missing implementation for TestSchemaExporter::Write",
            "Missing implementation for TestSchemaExporter::PostExport"])
        self.assertEqual(messages, expected)

        cmds.file(f=True, new=True)

    def testIOContextLister(self):
        """Testing that we can enumerate export contexts"""

        modes = set(cmds.mayaUSDListIOContexts(export=True))
        self.assertEqual(modes, set([
            "Null API Export", "Thierry", "Scene Grinder", "Curly's special",
            "Moe's special", "Larry's special", 'Bullet Physics API Export']))

        self.assertEqual(cmds.mayaUSDListIOContexts(exportOption="Null API Export"), "NullAPI")
        self.assertEqual(cmds.mayaUSDListIOContexts(exportAnnotation="Null API Export"),
                         "Exports an empty API for testing purpose")

    def testExportContextConflicts(self):
        """Testing that merging incompatible contexts generates errors"""
        mark = Tf.Error.Mark()
        mark.SetMark()
        self.assertTrue(mark.IsClean())

        cmds.polySphere(r=1)
        usdFilePath = os.path.abspath('UsdExportSchemaApiTestBasic.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath, 
                           extraContext=["Larry", "Curly", "Moe"])

        self.assertFalse(mark.IsClean())

        errors = mark.GetErrors()
        messages = set()
        for e in errors:
            messages.add(e.commentary)

        expected = set([
            "Arguments for context 'Larry' can not include extra contexts.",
            "Context 'Curly' and context 'Larry' do not agree on type of argument 'apiSchema'.",
            "Context 'Moe' and context 'Larry' do not agree on argument 'geomSidedness'."])
        self.assertEqual(messages, expected)

        cmds.file(f=True, new=True)

    @unittest.skipUnless(HAS_BULLET, 'Requires the bullet plugin.')  
    def testPluginSchemaAdaptors(self):
        """Testing a plugin Schema adaptor in a generic context:"""

        # Build a scene (and exercise the adaptor in a freeform context)
        cmds.file(f=True, new=True)

        s1T = cmds.polySphere()[0]
        cmds.loadPlugin("bullet")
        if not BulletUtils.checkPluginLoaded():
            return
        
        rbT, rbShape = RigidBody.CreateRigidBody().command(
            autoFit=True,
            colliderShapeType= RigidBody.eShapeType.kColliderSphere,
            meshes=[s1T],
            radius=1.0,
            mass=5.0,
            centerOfMass=(0.9, 0.8, 0.7))

        # See if the plugin adaptor can read the bullet shape under the mesh:
        sl = om.MSelectionList()
        sl.add(s1T)
        dagPath = sl.getDagPath(0)
        dagPath.extendToShape()

        adaptor = UsdMaya.Adaptor(dagPath.fullPathName())
        self.assertEqual(adaptor.GetUsdType(), Tf.Type.FindByName('UsdGeomMesh'))
        self.assertEqual(adaptor.GetAppliedSchemas(), ['PhysicsMassAPI'])
        physicsMass = adaptor.GetSchemaByName("PhysicsMassAPI")
        self.assertEqual(physicsMass.GetName(), "PhysicsMassAPI")
        massAttributes = set(['physics:centerOfMass',
                              'physics:density',
                              'physics:diagonalInertia',
                              'physics:mass',
                              'physics:principalAxes'])
        self.assertEqual(set(physicsMass.GetAttributeNames()), massAttributes)
        bulletAttributes = set(['physics:centerOfMass', 'physics:mass'])
        self.assertEqual(set(physicsMass.GetAuthoredAttributeNames()), bulletAttributes)
        bulletMass = physicsMass.GetAttribute('physics:mass')
        self.assertAlmostEqual(bulletMass.Get(), 5.0)
        bulletMass.Set(12.0)

        bulletCenter = physicsMass.GetAttribute('physics:centerOfMass')
        bulletCenter.Set(Gf.Vec3f(3, 4, 5))

        sl = om.MSelectionList()
        sl.add(s1T)
        bulletPath = sl.getDagPath(0)
        bulletPath.extendToShape(1)
        massDepFn = om.MFnDependencyNode(bulletPath.node())
        plug = om.MPlug(bulletPath.node(), massDepFn.attribute("mass"))
        self.assertAlmostEqual(plug.asFloat(), 12.0)

        # Create an untranslated attribute:
        usdDensity = physicsMass.CreateAttribute('physics:density')
        usdDensity.Set(33.0)
        self.assertAlmostEqual(usdDensity.Get(), 33.0)

        # This will result in a dynamic attribute on the bulletShape:
        plug = massDepFn.findPlug("USD_ATTR_physics_density", True)
        self.assertAlmostEqual(plug.asFloat(), 33.0)
        bulletAttributes.add('physics:density')
        self.assertEqual(set(physicsMass.GetAuthoredAttributeNames()), bulletAttributes)

        physicsMass.RemoveAttribute('physics:density')
        bulletAttributes.remove('physics:density')
        self.assertEqual(set(physicsMass.GetAuthoredAttributeNames()), bulletAttributes)

        # Try applying the schema on a new sphere:
        s2T = cmds.polySphere()[0]
        sl.add(s2T)
        dagPath = sl.getDagPath(1)
        dagPath.extendToShape()
        adaptor = UsdMaya.Adaptor(dagPath.fullPathName())
        physicsMass = adaptor.ApplySchemaByName("PhysicsMassAPI")
        self.assertEqual(physicsMass.GetName(), "PhysicsMassAPI")
        self.assertEqual(adaptor.GetUsdType(), Tf.Type.FindByName('UsdGeomMesh'))
        self.assertEqual(adaptor.GetAppliedSchemas(), ['PhysicsMassAPI'])

        usdDensity = physicsMass.CreateAttribute('physics:density')
        usdDensity.Set(33.0)

        # Export, but without enabling Bullet:
        usdFilePath = os.path.abspath('UsdExportSchemaApiTest_NoBullet.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath)

        # Check that there are no Physics API schemas exported:
        stage = Usd.Stage.Open(usdFilePath)
        for i in (1,2):
            spherePrim = stage.GetPrimAtPath('/pSphere{0}/pSphereShape{0}'.format(i))
            self.assertFalse("PhysicsMassAPI" in spherePrim.GetAppliedSchemas())

        # Export, with Bullet:
        usdFilePath = os.path.abspath('UsdExportSchemaApiTest_WithBullet.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
                           extraContext=["Bullet"])

        # Check that Physics API schemas did get exported:
        stage = Usd.Stage.Open(usdFilePath)
        values = [
            ("physics:centerOfMass", (Gf.Vec3f(3, 4, 5), Gf.Vec3f(0, 0, 0))),
            ("physics:mass", (12.0, 1.0)),
            ("physics:density", (None, 33.0)),
        ]
        for i in (1,2):
            spherePrim = stage.GetPrimAtPath('/pSphere{0}/pSphereShape{0}'.format(i))
            self.assertTrue("PhysicsMassAPI" in spherePrim.GetAppliedSchemas())
            for n,v in values:
                if v[i-1]:
                    a = spherePrim.GetAttribute(n)
                    self.assertEqual(a.Get(), v[i-1])


        # Try unapplying the schema:
        adaptor.UnapplySchemaByName("PhysicsMassAPI")
        self.assertEqual(adaptor.GetAppliedSchemas(), [])


if __name__ == '__main__':
    unittest.main(verbosity=2)
