#!/usr/bin/env mayapy
#
# Copyright 2018 Pixar
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

from pxr import Sdf
from pxr import Tf
from pxr import UsdGeom
from pxr import UsdSkel

from maya import cmds
from maya import standalone

import unittest

import fixturesUtils


class testUsdMayaAdaptor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Note that we do *not* explicitly load any USD plugins in this test.
        # We are testing that the Plug-based lookup mechanism correctly
        # identifies and loads the necessary libraries when a schema adapter is
        # requested.
        cls.inputPath = fixturesUtils.setUpClass(__file__, loadPlugin=False)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testApplySchema(self):
        """Tests schema application."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        proxy = mayaUsdLib.Adaptor('group1')
        self.assertTrue(proxy)
        self.assertEqual(proxy.GetAppliedSchemas(), [])

        modelAPI = proxy.ApplySchema(UsdGeom.ModelAPI)
        self.assertTrue(modelAPI)
        self.assertEqual(proxy.GetAppliedSchemas(), ["GeomModelAPI"])
        self.assertEqual(modelAPI.GetAuthoredAttributeNames(), [])

        motionAPI = proxy.ApplySchema(UsdGeom.MotionAPI)
        self.assertTrue(motionAPI)
        self.assertEqual(proxy.GetAppliedSchemas(),
                ["GeomModelAPI", "MotionAPI"])
        self.assertEqual(motionAPI.GetAuthoredAttributeNames(), [])

        # No support currently for typed schemas.
        with self.assertRaises(Tf.ErrorException):
            proxy.ApplySchema(UsdGeom.Xformable)

        with self.assertRaises(Tf.ErrorException):
            proxy.ApplySchema(UsdGeom.Mesh)

        self.assertEqual(proxy.GetAppliedSchemas(),
                ["GeomModelAPI", "MotionAPI"])

    def testGetSchema(self):
        """Tests getting schemas from the proxy."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        proxy = mayaUsdLib.Adaptor('group1')
        self.assertTrue(proxy.GetSchema(UsdGeom.ModelAPI))
        self.assertTrue(proxy.GetSchema(UsdGeom.Xform))

        # Wrong schemas should be invalid. Currently, exact schema match is
        # required (i.e. if Xform is compatible, Xformable isn't).
        self.assertFalse(proxy.GetSchema(UsdGeom.Mesh))

        # We can't currently get prim definitions from the schema registry for
        # typed non-concrete schemas, so we can't expect them to work with
        # GetSchema/GetSchemaByName.
        self.assertFalse(proxy.GetSchema(UsdGeom.Xformable))

    def testUnapplySchema(self):
        """Tests unapplying schemas and effect on existing proxy objects."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        proxy = mayaUsdLib.Adaptor('group1')
        self.assertEqual(proxy.GetAppliedSchemas(), [])

        proxy.ApplySchema(UsdGeom.ModelAPI)
        proxy.ApplySchema(UsdGeom.MotionAPI)
        self.assertEqual(proxy.GetAppliedSchemas(),
                ["GeomModelAPI", "MotionAPI"])

        proxy.UnapplySchema(UsdGeom.ModelAPI)
        self.assertEqual(proxy.GetAppliedSchemas(), ["MotionAPI"])

        # Note that schema objects still remain alive after being
        # unapplied.
        modelAPI = proxy.GetSchema(UsdGeom.ModelAPI)
        self.assertTrue(modelAPI)

        # But they expire once the underlying Maya node is removed.
        cmds.delete("group1")
        self.assertFalse(modelAPI)
        self.assertFalse(proxy)

    def testAttributes(self):
        """Tests creating and removing schema attributes."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        modelAPI = mayaUsdLib.Adaptor('group1').ApplySchema(UsdGeom.ModelAPI)
        xform = mayaUsdLib.Adaptor('group1').GetSchema(UsdGeom.Xform)

        # Schema attributes list versus authored attributes list.
        self.assertIn(UsdGeom.Tokens.modelCardTextureXPos,
                modelAPI.GetAttributeNames())
        self.assertNotIn(UsdGeom.Tokens.modelCardTextureXPos,
                modelAPI.GetAuthoredAttributeNames())
        self.assertIn(UsdGeom.Tokens.purpose,
                xform.GetAttributeNames())
        self.assertNotIn(UsdGeom.Tokens.purpose,
                xform.GetAuthoredAttributeNames())

        # Unauthored API attribute.
        self.assertFalse(
                modelAPI.GetAttribute(UsdGeom.Tokens.modelCardTextureXPos))

        # Unauthored schema attribute.
        self.assertFalse(
                xform.GetAttribute(UsdGeom.Tokens.purpose))

        # Non-existent attribute.
        with self.assertRaises(Tf.ErrorException):
            modelAPI.GetAttribute("fakeAttr")

        # Create and set an API attribute.
        attr = modelAPI.CreateAttribute(UsdGeom.Tokens.modelCardTextureXPos)
        self.assertTrue(attr)
        self.assertTrue(attr.Set(Sdf.AssetPath("example.png")))

        attr = modelAPI.GetAttribute(UsdGeom.Tokens.modelCardTextureXPos)
        self.assertTrue(attr)
        self.assertEqual(attr.Get(), Sdf.AssetPath("example.png"))

        self.assertEqual(attr.GetAttributeDefinition().name,
                "model:cardTextureXPos")
        self.assertEqual(modelAPI.GetAuthoredAttributeNames(),
                [UsdGeom.Tokens.modelCardTextureXPos])

        # Existing attrs become invalid when the plug is removed.
        modelAPI.RemoveAttribute(UsdGeom.Tokens.modelCardTextureXPos)
        self.assertFalse(attr)

        # Should return None on invalid attr access.
        self.assertIsNone(attr.Get())

        # Create a typed schema attribute. It should be set to the fallback
        # initially.
        self.assertEqual(xform.CreateAttribute(UsdGeom.Tokens.purpose).Get(),
                UsdGeom.Tokens.default_)

    def testMetadata(self):
        """Tests setting and clearing metadata."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        proxy = mayaUsdLib.Adaptor('group1')
        self.assertEqual(proxy.GetAllAuthoredMetadata(), {})
        self.assertIsNone(proxy.GetMetadata("instanceable"))

        proxy.SetMetadata("instanceable", True)
        self.assertEqual(proxy.GetMetadata("instanceable"), True)

        proxy.SetMetadata("kind", "awesomeKind")
        self.assertEqual(proxy.GetAllAuthoredMetadata(), {
            "instanceable": True,
            "kind": "awesomeKind"
        })

        # Unregistered key.
        with self.assertRaises(Tf.ErrorException):
            proxy.SetMetadata("fakeMetadata", True)

        with self.assertRaises(Tf.ErrorException):
            proxy.GetMetadata("otherFakeMetadata")

        proxy.ApplySchema(UsdGeom.ModelAPI)
        apiSchemas = proxy.GetMetadata("apiSchemas")
        self.assertEqual(apiSchemas.prependedItems, ["GeomModelAPI"])

        # Clear metadata.
        proxy.ClearMetadata("kind")
        self.assertIsNone(proxy.GetMetadata("kind"))

    def testStaticHelpers(self):
        """Tests the static helpers for querying metadata and schema info."""
        self.assertIn("MotionAPI", mayaUsdLib.Adaptor.GetRegisteredAPISchemas())
        self.assertIn("hidden", mayaUsdLib.Adaptor.GetPrimMetadataFields())

    def testAttributeAliases(self):
        """Tests behavior with the purpose/USD_purpose alias."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        cmds.addAttr("group1", longName="USD_purpose", dataType="string")
        cmds.setAttr("group1.USD_purpose", "proxy", type="string")
        self.assertEqual(
                mayaUsdLib.Adaptor("group1")
                    .GetSchema(UsdGeom.Xform)
                    .GetAttribute(UsdGeom.Tokens.purpose)
                    .Get(),
                UsdGeom.Tokens.proxy)

        cmds.addAttr("group1", longName="USD_ATTR_purpose",
                dataType="string")
        cmds.setAttr("group1.USD_ATTR_purpose", "render", type="string")
        self.assertEqual(
                mayaUsdLib.Adaptor("group1")
                    .GetSchema(UsdGeom.Xform)
                    .GetAttribute(UsdGeom.Tokens.purpose)
                    .Get(),
                UsdGeom.Tokens.render)

        cmds.deleteAttr("group1.USD_purpose")
        self.assertEqual(
                mayaUsdLib.Adaptor("group1")
                    .GetSchema(UsdGeom.Xform)
                    .GetAttribute(UsdGeom.Tokens.purpose)
                    .Get(),
                UsdGeom.Tokens.render)

        cmds.deleteAttr("group1.USD_ATTR_purpose")
        self.assertIsNone(
                mayaUsdLib.Adaptor("group1")
                    .GetSchema(UsdGeom.Xform)
                    .GetAttribute(UsdGeom.Tokens.purpose)
                    .Get())

        mayaUsdLib.Adaptor("group1")\
                    .GetSchema(UsdGeom.Xform)\
                    .CreateAttribute(UsdGeom.Tokens.purpose)
        self.assertTrue(cmds.attributeQuery("USD_ATTR_purpose", node="group1",
                exists=True))

    def testConcreteSchemaRegistrations(self):
        """Tests some of the PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA macros."""
        cmds.file(new=True, force=True)
        cmds.createNode("joint", name="TestJoint")
        self.assertTrue(
                mayaUsdLib.Adaptor("TestJoint").GetSchema(UsdSkel.Skeleton))
        self.assertFalse(
                mayaUsdLib.Adaptor("TestJoint").GetSchema(UsdGeom.Mesh))

        cmds.createNode("camera", name="TestCamera")
        self.assertTrue(
                mayaUsdLib.Adaptor("TestCamera").GetSchema(UsdGeom.Camera))

        cmds.createNode("mesh", name="TestMesh")
        self.assertTrue(
                mayaUsdLib.Adaptor("TestMesh").GetSchema(UsdGeom.Mesh))

        cmds.createNode("instancer", name="TestInstancer")
        self.assertTrue(
                mayaUsdLib.Adaptor("TestInstancer").GetSchema(
                UsdGeom.PointInstancer))

        cmds.createNode("nurbsSurface", name="TestNurbsSurface")
        self.assertTrue(
                mayaUsdLib.Adaptor("TestNurbsSurface").GetSchema(
                UsdGeom.NurbsPatch))

        cmds.createNode("nurbsCurve", name="TestNurbsCurve")
        self.assertTrue(
                mayaUsdLib.Adaptor("TestNurbsCurve").GetSchema(
                UsdGeom.NurbsCurves))

        cmds.createNode("locator", name="TestLocator")
        self.assertTrue(
                mayaUsdLib.Adaptor("TestLocator").GetSchema(UsdGeom.Xform))

        cmds.createNode("nParticle", name="TestParticles")
        self.assertTrue(
                mayaUsdLib.Adaptor("TestParticles").GetSchema(UsdGeom.Points))

    def testGetAttributeAliases(self):
        """Tests the GetAttributeAliases function."""
        self.assertEqual(
                mayaUsdLib.Adaptor.GetAttributeAliases("subdivisionScheme"),
                ["USD_ATTR_subdivisionScheme", "USD_subdivisionScheme"])

    def testAdaptorRegistration(self):
        """Tests Registration Python functions."""
        mayaUsdLib.Adaptor.RegisterTypedSchemaConversion("polySphere", UsdGeom.Sphere)
        mayaUsdLib.Adaptor.RegisterAttributeAlias(UsdGeom.Tokens.faceVaryingLinearInterpolation,"USD_RegTest")
        cmds.file(new=True, force=True)
        cmds.createNode("polySphere", name="TestSphere")
        self.assertTrue(
                mayaUsdLib.Adaptor("TestSphere").GetSchema(UsdGeom.Sphere))
        self.assertEqual(
                mayaUsdLib.Adaptor.GetAttributeAliases("faceVaryingLinearInterpolation"),
                ["USD_ATTR_faceVaryingLinearInterpolation", "USD_RegTest", "USD_faceVaryingLinearInterpolation"])

if __name__ == '__main__':
    unittest.main(verbosity=2)
