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

from pxr import Tf, Gf, Usd

from maya import cmds
import maya.api.OpenMaya as om
import maya.api.OpenMayaAnim as oma
from maya import standalone

HAS_USDMAYA = False
try:
    from pxr import UsdMaya
    HAS_USDMAYA = True
except ImportError as ie:
    pass

HAS_BULLET = False
try:
    from pxr import UsdPhysics
    import maya.app.mayabullet.BulletUtils as BulletUtils
    import maya.app.mayabullet.RigidBody as RigidBody
    HAS_BULLET = True
except ImportError as ie:
    pass

import fixturesUtils, os

import unittest

class shadowApiAdaptorShape(mayaUsdLib.SchemaApiAdaptor):
    _nameMapping = {
        "inputs:shadow:color": "shadowColor",
        # Two Maya enablers. Return the RayTrace one.
        "inputs:shadow:enable": "useRayTraceShadows"
    }
    def CanAdapt(self):
        node = om.MFnDependencyNode(self.mayaObject)
        # Add a name check so we can test a bit more API
        if node.name() == "pointLightShape1":
            return True
        else:
            return False

    def GetAdaptedAttributeNames(self):
        return list(shadowApiAdaptorShape._nameMapping.keys())

    def GetMayaNameForUsdAttrName(self, usdName):
        return shadowApiAdaptorShape._nameMapping.get(usdName, "")


def _GetBulletShape(mayaShape):
    """Navigates from a Maya shape to a bullet shape under the same transform"""
    path = om.MDagPath.getAPathTo(mayaShape)
    if not path or not path.pop():
        return None

    for i in range(path.numberOfShapesDirectlyBelow()):
        path.extendToShape(i)
        depFn = om.MFnDependencyNode(path.node())
        if depFn.typeName == "bulletRigidBodyShape":
            return path.node()
        path.pop()

    return None

def _ApplyBulletSchema(mayaShape):
    """Creates a bullet simulation containing the "mayaShape", this is obviously demo
    code, so we will not even try to add the shape to an existing simluation."""
    if _GetBulletShape(mayaShape) is not None:
        return True

    path = om.MDagPath.getAPathTo(mayaShape)

    if not path or not path.pop():
        return False

    BulletUtils.checkPluginLoaded()
    RigidBody.CreateRigidBody.command(transformName=path.fullPathName(), bAttachSelected=False)

    return True


class TestBulletMassShemaAdaptor(mayaUsdLib.SchemaApiAdaptor):
    _nameMapping = {
        "physics:mass": "mass",
        "physics:centerOfMass": "centerOfMass"
    }
    def CanAdapt(self):
        # We do not want to process the bullet node shape itself. It adds nothing of interest.
        if not self.mayaObject:
            return False

        node = om.MFnDependencyNode(self.mayaObject)
        if not node or node.typeName == "bulletRigidBodyShape":
            return False

        return self.GetMayaObjectForSchema() is not None

    def CanAdaptForImport(self, jobArgs):
        return "PhysicsMassAPI" in jobArgs.includeAPINames

    def CanAdaptForExport(self, jobArgs):
        if "PhysicsMassAPI" in jobArgs.includeAPINames:
            return self.CanAdapt()
        return False

    def ApplySchemaForImport(self, primReaderArgs, context):
        # Check if already applied:
        if self.GetMayaObjectForSchema() is not None:
            return True

        retVal = self.ApplySchema(om.MDGModifier())

        if retVal:
            newObject = self.GetMayaObjectForSchema()
            if newObject is None:
                return False

            depFn = om.MFnDependencyNode(newObject)

            # Register the new node:
            nodePath = primReaderArgs.GetUsdPrim().GetPath().pathString + "/" + depFn.name()

            context.RegisterNewMayaNode(nodePath, newObject)

        return retVal

    def ApplySchema(self, dgModifier):
        if _ApplyBulletSchema(self.mayaObject):
            return _GetBulletShape(self.mayaObject) is not None
        return False

    def UnapplySchema(self, dgModifier):
        if self.GetMayaObjectForSchema() is None:
            # Already unapplied?
            return False

        path = om.MDagPath.getAPathTo(self.mayaObject)
        if not path or not path.pop():
            return False

        BulletUtils.checkPluginLoaded()
        BulletUtils.removeBulletObjectsFromList([path.fullPathName()])

        return self.GetMayaObjectForSchema() is None

    def disabled_UndoableCreateAttribute(self, attrName, dgModifier):
        """Disabling until we figure out why the superclass can not be called"""
        retVal = super(TestBulletMassShemaAdaptor, self).UndoableCreateAttribute(attrName, dgModifier)
        print("UndoableCreateAttribute called for", attrName)
        return retVal

    def disabled_UndoableRemoveAttribute(self, attrName, dgModifier):
        """Disabling until we figure out why the superclass can not be called"""
        retVal = super(TestBulletMassShemaAdaptor, self).UndoableRemoveAttribute(attrName, dgModifier)
        print("UndoableRemoveAttribute called for", attrName)
        return retVal

    def GetMayaObjectForSchema(self):
        return _GetBulletShape(self.mayaObject)

    def GetAdaptedAttributeNames(self):
        return list(TestBulletMassShemaAdaptor._nameMapping.keys())

    def GetMayaNameForUsdAttrName(self, usdName):
        return TestBulletMassShemaAdaptor._nameMapping.get(usdName, "")


class TestBulletRigidBodyShemaAdaptor(mayaUsdLib.SchemaApiAdaptor):
    def CanAdapt(self):
        # This class does not adapt in a freeform context.
        return False

    def CanAdaptForImport(self, jobArgs):
        if "PhysicsRigidBodyAPI" not in jobArgs.includeAPINames or not self.mayaObject:
            return False

        node = om.MFnDependencyNode(self.mayaObject)
        if not node or node.typeName == "bulletRigidBodyShape":
            return False

        return True

    def CanAdaptForExport(self, jobArgs):
        return self.CanAdaptForImport(jobArgs) and self.GetMayaObjectForSchema() is not None

    def ApplySchemaForImport(self, primReaderArgs, context):
        # Check if already applied:
        if self.GetMayaObjectForSchema() is not None:
            return True

        retVal = _ApplyBulletSchema(self.mayaObject)

        if retVal:
            newObject = self.GetMayaObjectForSchema()
            if newObject is None:
                return False

            depFn = om.MFnDependencyNode(newObject)

            # Register the new node:
            nodePath = primReaderArgs.GetUsdPrim().GetPath().pathString + "/" + depFn.name()

            context.RegisterNewMayaNode(nodePath, newObject)

        return retVal

    def GetMayaObjectForSchema(self):
        return _GetBulletShape(self.mayaObject)

    def CopyToPrim(self, prim, usdTime, valueWriter):
        rbSchema = UsdPhysics.RigidBodyAPI.Apply(prim)
        velAttr = rbSchema.CreateVelocityAttr()
        depFn = om.MFnDependencyNode(self.GetMayaObjectForSchema())
        velocityPlug = depFn.findPlug("initialVelocity", False)

        mayaUsdLib.WriteUtil.SetUsdAttr(velocityPlug, velAttr, usdTime, valueWriter)

        return True

    def CopyFromPrim(self, prim, args, context):
        rbSchema = UsdPhysics.RigidBodyAPI(prim)
        if not rbSchema:
            return False

        velAttr = rbSchema.GetVelocityAttr()
        if velAttr:
            mayaUsdLib.ReadUtil.ReadUsdAttribute(
                velAttr, self.GetMayaObjectForSchema(),
                "initialVelocity", args, context)

        return True


class testSchemaApiAdaptor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._rootPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        pass

    @unittest.skipUnless(Usd.GetVersion() > (0, 21, 2), 'USD Lux becomes connectable in 21.05.')
    def testMinimalAdaptation(self):
        """Test that we can adapt ShadowAPI to an existing light shape. This exercises the most
        basic callbacks exposed by a SchemaAPI adaptor:"""

        mayaUsdLib.SchemaApiAdaptor.Register(shadowApiAdaptorShape, "light", "ShadowAPI")

        lightShape1 = cmds.pointLight()
        cmds.setAttr(lightShape1 + ".shadowColor", 0.5, 0.25, 0)
        lightShape2 = cmds.pointLight()
        cubeXform, cubeShape = cmds.polyCube()

        # Wrong type: not adapted
        adaptor = mayaUsdLib.Adaptor(cubeShape)
        self.assertEqual(adaptor.GetAppliedSchemas(), [])

        # Wrong name: not adapted
        adaptor = mayaUsdLib.Adaptor(lightShape2)
        self.assertEqual(adaptor.GetAppliedSchemas(), [])

        # Adapted:
        adaptor = mayaUsdLib.Adaptor(lightShape1)
        self.assertEqual(adaptor.GetAppliedSchemas(), ["ShadowAPI"])

        schema = adaptor.GetSchemaByName("ShadowAPI")
        self.assertTrue(schema)

        self.assertEqual(set(schema.GetAuthoredAttributeNames()),
                         set(["inputs:shadow:color", "inputs:shadow:enable"]))
        colorAttr = schema.GetAttribute("inputs:shadow:color")
        self.assertTrue(colorAttr)
        linearizedValue = (0.21763764, 0.047366142, 0)
        colorAttrValue = colorAttr.Get()
        self.assertAlmostEqual(linearizedValue[0], colorAttrValue[0])
        self.assertAlmostEqual(linearizedValue[1], colorAttrValue[1])
        self.assertEqual(linearizedValue[2], colorAttrValue[2])

        colorAttr.Set((1,0,1))
        self.assertEqual(cmds.getAttr(lightShape1 + ".shadowColor"), [(1.0, 0.0, 1.0)])

    @unittest.skipUnless(HAS_BULLET and HAS_USDMAYA, 'Requires the Pixar and bullet plugins.')
    def testComplexAdaptation(self):
        """Test that we can adapt a bullet simulation"""

        mayaUsdLib.SchemaApiAdaptor.Register(TestBulletMassShemaAdaptor, "shape", "PhysicsMassAPI")
        mayaUsdLib.SchemaApiAdaptor.Register(TestBulletRigidBodyShemaAdaptor, "shape", "PhysicsRigidBodyAPI")

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

        adaptor = mayaUsdLib.Adaptor(dagPath.fullPathName())
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

        schemasToExport = ["PhysicsMassAPI", "PhysicsRigidBodyAPI"]
        # Export, with Bullet:
        usdFilePath = os.path.abspath('UsdExportSchemaApiTest_WithBullet.usda')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFilePath,
                           apiSchema=schemasToExport, frameRange=(1, 10))

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
                numberOfExportedKeys = len(a.GetTimeSamples())

        # Try unapplying the schema:
        adaptor.UnapplySchemaByName("PhysicsMassAPI")
        self.assertEqual(adaptor.GetAppliedSchemas(), [])

        # Test import of USDPhysics without job context:
        cmds.file(new=True, force=True)
        cmds.mayaUSDImport(f=usdFilePath)

        sl = om.MSelectionList()
        # pSphereShape1 is a transform, since the bullet shape prevented merging the mesh and the
        # transform. The shape will be pSphereShape1Shape...
        sl.add("pSphereShape1")
        bulletPath = sl.getDagPath(0)
        # No bullet shape since we did not put Bullet as jobContext
        self.assertEqual(bulletPath.numberOfShapesDirectlyBelow(), 1)

        cmds.file(new=True, force=True)
        cmds.mayaUSDImport(f=usdFilePath, apiSchema=schemasToExport, readAnimData=True)

        sl = om.MSelectionList()
        sl.add("pSphereShape1")
        bulletPath = sl.getDagPath(0)
        # Finds bullet shape since we did put Bullet as jobContext

        self.assertEqual(bulletPath.numberOfShapesDirectlyBelow(), 2)

        # The bullet shape has animated mass and initial velocity since we read the animation.
        bulletPath.extendToShape(1)
        massDepFn = om.MFnDependencyNode(bulletPath.node())
        for attrName in ("mass", "initialVelocityX", "initialVelocityY", "initialVelocityZ"):
            plug = om.MPlug(bulletPath.node(), massDepFn.attribute(attrName))
            self.assertTrue(plug.isConnected)
            fcurve = oma.MFnAnimCurve(plug.source().node())
            self.assertEqual(fcurve.numKeys, numberOfExportedKeys)


if __name__ == '__main__':
    unittest.main(verbosity=2)
