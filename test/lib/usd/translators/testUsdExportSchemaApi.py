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

    def testExportJobContextLister(self):
        """Testing that we can enumerate export contexts"""

        modes = set(cmds.mayaUSDListJobContexts(export=True))
        self.assertEqual(modes, set([
            "Null API", "Thierry", "Scene Grinder", "Curly's special",
            "Moe's special", "Larry's special", 'Bullet Physics API Export']))

        self.assertEqual(cmds.mayaUSDListJobContexts(jobContext="Null API"), "NullAPI")
        self.assertEqual(cmds.mayaUSDListJobContexts(exportAnnotation="Null API"),
                         "Exports an empty API for testing purpose")
        self.assertEqual(cmds.mayaUSDListJobContexts(exportArguments="Null API"), 
            'apiSchema=[testApi];chaser=[NullAPIChaser];chaserArgs=[[NullAPIChaser,life,42]];')
        self.assertEqual(cmds.mayaUSDListJobContexts(importAnnotation="Null API"),
                         "Imports an empty API for testing purpose")
        self.assertEqual(cmds.mayaUSDListJobContexts(importArguments="Null API"), 
            'apiSchema=[testApiIn];chaser=[NullAPIChaserIn];chaserArgs=[[NullAPIChaserIn,universe,42]];')

    def testExportJobContextConflicts(self):
        """Testing that merging incompatible contexts generates errors"""
        mark = Tf.Error.Mark()
        mark.SetMark()
        self.assertTrue(mark.IsClean())

        cmds.polySphere(r=1)
        usdFilePath = os.path.abspath('UsdExportSchemaApiTestBasic.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath, 
                           jobContext=["Larry", "Curly", "Moe"])

        self.assertFalse(mark.IsClean())

        errors = mark.GetErrors()
        messages = set()
        for e in errors:
            messages.add(e.commentary)

        expected = set([
            "Arguments for job context 'Larry' can not include extra contexts.",
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
        # NOTICE: PhysicsRigidBodyAPI is not in the list because that API is
        # supported only on export!!!
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

        # Add some animation:
        cmds.setKeyframe(bulletPath, at="mass", t=0, v=3.0)
        cmds.setKeyframe(bulletPath, at="mass", t=10, v=30.0)

        # Modify the velocity so it can be exported to the RBD Physics schema.
        cmds.setKeyframe(bulletPath, at="initialVelocityX", t=0, v=5.0)
        cmds.setKeyframe(bulletPath, at="initialVelocityX", t=10, v=50.0)
        cmds.setKeyframe(bulletPath, at="initialVelocityY", t=0, v=6.0)
        cmds.setKeyframe(bulletPath, at="initialVelocityY", t=10, v=60.0)
        cmds.setKeyframe(bulletPath, at="initialVelocityZ", t=0, v=7.0)
        cmds.setKeyframe(bulletPath, at="initialVelocityZ", t=10, v=70.0)

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
                           jobContext=["Bullet"], frameRange=(1, 10))

        # Check that Physics API schemas did get exported:
        stage = Usd.Stage.Open(usdFilePath)
        values = [
            ("physics:centerOfMass", (Gf.Vec3f(3, 4, 5), Gf.Vec3f(0, 0, 0))),
            ("physics:mass", (3.0, 1.0)),
            ("physics:density", (None, 33.0)),
        ]
        for i in (1,2):
            spherePrim = stage.GetPrimAtPath('/pSphere{0}/pSphereShape{0}'.format(i))
            self.assertTrue("PhysicsMassAPI" in spherePrim.GetAppliedSchemas())
            for n,v in values:
                if v[i-1]:
                    a = spherePrim.GetAttribute(n)
                    self.assertEqual(a.Get(), v[i-1])
            if i == 1:
                # Is mass animated?
                a = spherePrim.GetAttribute("physics:mass")
                self.assertEqual(a.Get(10), 30)
            # This got exported even though invisible in interactive:
            self.assertTrue("PhysicsRigidBodyAPI" in spherePrim.GetAppliedSchemas())
            a = spherePrim.GetAttribute("physics:velocity")
            if i == 1:
                self.assertEqual(a.Get(0), (5, 6, 7))
                self.assertEqual(a.Get(10), (50, 60, 70))



        # Try unapplying the schema:
        adaptor.UnapplySchemaByName("PhysicsMassAPI")
        self.assertEqual(adaptor.GetAppliedSchemas(), [])


if __name__ == '__main__':
    unittest.main(verbosity=2)
