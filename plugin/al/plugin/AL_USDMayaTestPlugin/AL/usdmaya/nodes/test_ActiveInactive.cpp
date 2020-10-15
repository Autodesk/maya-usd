//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "test_usdmaya.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <maya/MDagModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>

#include <fstream>

#include <stdio.h>

using AL::maya::test::buildTempPath;

const MString temp_path_cube = buildTempPath("AL_USDMayaTests_cube.ma");
const MString temp_path_sphere = buildTempPath("AL_USDMayaTests_sphere.ma");
const MString temp_path_camera = buildTempPath("AL_USDMayaTests_camera.ma");

static const MString g_inactive = MString("#usda 1.0\n"
                                          "\n"
                                          "def Xform \"root\"\n"
                                          "{\n"
                                          "    def ALMayaReference \"rig\" (\n"
                                          "      active = false\n"
                                          "    )\n"
                                          "    {\n"
                                          "      asset mayaReference = \"")
    + temp_path_cube
    + "\"\n"
      "      string mayaNamespace = \"cube\"\n"
      "    }\n"
      "}\n";

static const MString g_active = MString("#usda 1.0\n"
                                        "\n"
                                        "def Xform \"root\"\n"
                                        "{\n"
                                        "    def ALMayaReference \"rig\"\n"
                                        "    {\n"
                                        "      asset mayaReference = \"")
    + temp_path_cube
    + "\"\n"
      "      string mayaNamespace = \"cube\"\n"
      "    }\n"
      "}\n";

static const MString g_variants = MString("#usda 1.0\n"
                                          "(\n"
                                          "    defaultPrim = \"rig_variants\"\n"
                                          ")\n"
                                          "\n"
                                          "def Xform \"root\"\n"
                                          "(\n"
                                          "    variants = {\n"
                                          "        string rig_technical = \"sphere\"\n"
                                          "    }\n"
                                          "    add variantSets = \"rig_technical\"\n"
                                          ")\n"
                                          "{\n"
                                          "    variantSet \"rig_technical\" = {\n"
                                          "      \"sphere\"{\n"
                                          "        def ALMayaReference \"rig\"\n"
                                          "        {\n"
                                          "           asset mayaReference = \"")
    + temp_path_sphere
    + "\"\n"
      "           string mayaNamespace = \"dave\"\n"
      "        }\n"
      "      }\n"
      "      \"cube\"{\n"
      "        def ALMayaReference \"rig\"\n"
      "        {\n"
      "           asset mayaReference = \""
    + temp_path_cube
    + "\"\n"
      "           string mayaNamespace = \"dave\"\n"
      "        }\n"
      "      }\n"
      "      \"fredcube\"{\n"
      "        def ALMayaReference \"rig\"\n"
      "        {\n"
      "           asset mayaReference = \""
    + temp_path_cube
    + "\"\n"
      "           string mayaNamespace = \"fred\"\n"
      "        }\n"
      "      }\n"
      "      \"cache\"{\n"
      "        def Sphere \"rig\"\n"
      "        {\n"
      "          double radius = 1\n"
      "        }\n"
      "      }\n"
      "    }\n"
      "}\n";

static const MString g_customTransformType = MString("#usda 1.0\n"
                                                     "\n"
                                                     "def Xform \"root\"\n"
                                                     "{\n"
                                                     "    def ALMayaReference \"rig\" (\n"
                                                     "      al_usdmaya_transformType = \"joint\"\n"
                                                     "    )\n"
                                                     "    {\n"
                                                     "      asset mayaReference = \"")
    + temp_path_cube
    + "\"\n"
      "      string mayaNamespace = \"cube\"\n"
      "    }\n"
      "}\n";

static const MString g_duplicateTransformNames
    = MString("#usda 1.0\n"
              "\n"
              "def Xform \"root\"\n"
              "{\n"
              "  def Xform \"one\"\n"
              "  {\n"
              "    def ALMayaReference \"rig\" (\n"
              "      al_usdmaya_transformType = \"joint\"\n"
              "    )\n"
              "    {\n"
              "      asset mayaReference = \"")
    + temp_path_cube
    + "\"\n"
      "      string mayaNamespace = \"cube\"\n"
      "    }\n"
      "  }\n"
      "  def Xform \"two\"\n"
      "  {\n"
      "    def ALMayaReference \"rig\" (\n"
      "      al_usdmaya_transformType = \"joint\"\n"
      "    )\n"
      "    {\n"
      "      asset mayaReference = \""
    + temp_path_sphere
    + "\"\n"
      "      string mayaNamespace = \"cube\"\n"
      "    }\n"
      "  }\n"
      "}\n";

static const MString g_variantSwitchPrimTypes
    = MString("#usda 1.0\n"
              "\n"
              "def Xform \"root\"\n"
              "{\n"
              "    def Xform \"switchable\"(\n"
              "        variants = {\n"
              "            string option = \"camera\"\n"
              "        }\n"
              "        add variantSets = \"option\"\n"
              "    )\n"
              "    {\n"
              "        variantSet \"option\" = {\n"
              "            \"camera\" {\n"
              "                def  Xform \"top\"\n"
              "                {\n"
              "                    def  Camera \"cam\"\n"
              "                    {\n"
              "                    }\n"
              "                }\n"
              "            }\n"
              "            \"mayaReference\" {\n"
              "                def  ALMayaReference \"top\"\n"
              "                {\n"
              "                    asset mayaReference = @")
    + temp_path_camera
    + "@\n"
      "                    string mayaNamespace = \"cam_ns\"\n"
      "                }\n"
      "            }\n"
      "            \"no_translator\" {\n"
      "                def  Xform \"top\"\n"
      "                {\n"
      "                    def  Xform \"cam\"\n"
      "                    {\n"
      "                    }\n"
      "                }\n"
      "            }\n"
      "        }\n"
      "    }\n"
      "}\n";

class ActiveInactive : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        // pCube1, pCubeShape1, polyCube1
        MFileIO::newFile(true);
        MGlobal::executeCommand("polyCube -w 1 -h 1 -d 1 -sd 1 -sh 1 -sw 1", false, false);
        MFileIO::saveAs(temp_path_cube, 0, true);

        // pSphere1, pSphereShape1, polySphere1
        MFileIO::newFile(true);
        MGlobal::executeCommand("polySphere", false, false);
        MFileIO::saveAs(temp_path_sphere, 0, true);

        // camera1, camera1Shape
        MFileIO::newFile(true);
        MGlobal::executeCommand("camera", false, false);
        MGlobal::executeCommand("group -name camera_rigg_top camera1", false, false);
        MFileIO::saveAs(temp_path_camera, 0, true);
    }

    static void TearDownTestCase()
    {
        // cleanup files
        remove(temp_path_cube.asChar());
        remove(temp_path_sphere.asChar());
        remove(temp_path_camera.asChar());
    }

protected:
    virtual void SetUp()
    {
        // Clear the scene before the best
        MFileIO::newFile(true);
    }

    virtual void TearDown()
    {
        // empty
    }

private:
};

TEST_F(ActiveInactive, duplicateTransformNames)
{
    const std::string temp_path = buildTempPath("AL_USDMayaTests_duplicateTransformNames.usda");
    const std::string temp_ma_path = buildTempPath("AL_USDMayaTests_duplicateTransformNames.ma");

    // output a couple of usda files for testing (active and inactive)
    {
        std::ofstream os(temp_path);
        os << g_duplicateTransformNames;
    }

    {
        MString    shapeName;
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
        shapeName = fn.name();

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        // stage should be valid
        EXPECT_TRUE(stage);

        {
            MObject node1 = proxy->findRequiredPath(SdfPath("/root/one/rig"));
            EXPECT_NE(MObject::kNullObj, node1);
            EXPECT_EQ(MFn::kJoint, node1.apiType());

            MObject node2 = proxy->findRequiredPath(SdfPath("/root/two/rig"));
            EXPECT_NE(MObject::kNullObj, node2);
            EXPECT_EQ(MFn::kJoint, node2.apiType());
        }
    }
    {
        MFileIO::saveAs(temp_ma_path.c_str(), 0, true);
        MFileIO::newFile(true);
        MFileIO::open(temp_ma_path.c_str(), 0, true);

        // one of the prims should be a joint
        MItDependencyNodes it(MFn::kPluginShape);
        MFnDagNode         fn(it.item());

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        {
            MObject node1 = proxy->findRequiredPath(SdfPath("/root/one/rig"));
            EXPECT_NE(MObject::kNullObj, node1);
            EXPECT_EQ(MFn::kJoint, node1.apiType());

            MObject node2 = proxy->findRequiredPath(SdfPath("/root/two/rig"));
            EXPECT_NE(MObject::kNullObj, node2);
            EXPECT_EQ(MFn::kJoint, node2.apiType()) << node2.apiTypeStr() << " is not MFn::kJoint";

            EXPECT_NE(node1, node2);
        }
    }
}

TEST_F(ActiveInactive, customTransformType)
{
    const std::string temp_path = buildTempPath("AL_USDMayaTests_customTransformType.usda");
    const std::string temp_ma_path = buildTempPath("AL_USDMayaTests_customTransformType.ma");
    const std::string temp_ma_path2
        = buildTempPath("AL_USDMayaTests_customTransformTypeInactive.ma");

    // output a couple of usda files for testing (active and inactive)
    {
        std::ofstream os(temp_path);
        os << g_customTransformType;
    }

    {
        MString    shapeName;
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
        shapeName = fn.name();

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        // stage should be valid
        EXPECT_TRUE(stage);

        // should not be able to select the items in the reference file
        MSelectionList sl;
        EXPECT_TRUE(bool(sl.add("cube:pCube1")));
        EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();

        MFileIO::saveAs(temp_ma_path.c_str(), 0, true);

        // one of the prims should be a joint
        MObject node = proxy->findRequiredPath(SdfPath("/root/rig"));
        EXPECT_NE(MObject::kNullObj, node);
        EXPECT_EQ(MFn::kJoint, node.apiType());

        // deactivate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a false -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);

        EXPECT_FALSE(bool(sl.add("cube:pCube1")));
        EXPECT_FALSE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_FALSE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(0u, sl.length());

        MFileIO::saveAs(temp_ma_path2.c_str(), 0, true);

        // activate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a true -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);

        // should now be able to select the items in the reference file
        EXPECT_TRUE(bool(sl.add("cube:pCube1")));
        EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();
    }

    {
        MFileIO::newFile(true);
        MFileIO::open(temp_ma_path.c_str(), 0, true);

        // one of the prims should be a joint
        MItDependencyNodes it(MFn::kPluginShape);
        MFnDagNode         fn(it.item());

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
        MObject                         node = proxy->findRequiredPath(SdfPath("/root/rig"));
        EXPECT_NE(MObject::kNullObj, node);
        EXPECT_EQ(MFn::kJoint, node.apiType());

        // should not be able to select the items in the reference file
        MSelectionList sl;
        EXPECT_TRUE(bool(sl.add("cube:pCube1")));
        EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();

        // deactivate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a false -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);
        EXPECT_FALSE(bool(sl.add("cube:pCube1")));
        EXPECT_FALSE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_FALSE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(0u, sl.length());

        // activate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a true -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);
        EXPECT_TRUE(bool(sl.add("cube:pCube1")));
        EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();
    }

    {
        MFileIO::newFile(true);
        MFileIO::open(temp_ma_path2.c_str(), 0, true);

        // one of the prims should be a joint
        MItDependencyNodes it(MFn::kPluginShape);
        MFnDagNode         fn(it.item());

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
        MObject                         node = proxy->findRequiredPath(SdfPath("/root/rig"));
        // inactive prim should not be "seen" by AL_USDMaya
        EXPECT_EQ(MObject::kNullObj, node);

        // should not be able to select the items in the reference file
        MSelectionList sl;
        EXPECT_FALSE(bool(sl.add("cube:pCube1")));
        EXPECT_FALSE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_FALSE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(0u, sl.length());

        // activate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a true -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);
        EXPECT_TRUE(bool(sl.add("cube:pCube1")));
        EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();

        // deactivate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a false -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);
        EXPECT_FALSE(bool(sl.add("cube:pCube1")));
        EXPECT_FALSE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_FALSE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(0u, sl.length());
    }
}

TEST_F(ActiveInactive, disable)
{
    const std::string temp_path = buildTempPath("AL_USDMayaTests_activePrim.usda");
    const std::string temp_path2 = buildTempPath("AL_USDMayaTests_inactivePrim.usda");
    const std::string temp_path3 = buildTempPath("AL_USDMayaTests_variants.usda");
    const std::string temp_ma_path = buildTempPath("AL_USDMayaTests_customTransformType.ma");
    const std::string temp_ma_path2
        = buildTempPath("AL_USDMayaTests_customTransformTypeInactive.ma");
    const std::string temp_ma_path3 = buildTempPath("AL_USDMayaTests_inactivePrim.ma");

    // output a couple of usda files for testing (active and inactive)
    {
        std::ofstream os(temp_path);
        os << g_active;
    }
    {
        std::ofstream os(temp_path2);
        os << g_inactive;
    }
    {
        std::ofstream os(temp_path3);
        os << g_variants;
    }

    {
        MString    shapeName;
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
        shapeName = fn.name();

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path2.c_str());

        auto stage = proxy->getUsdStage();

        // stage should be valid
        EXPECT_TRUE(stage);

        // should not be able to select the items in the reference file
        MSelectionList sl;
        EXPECT_FALSE(bool(sl.add("cube:pCube1")));
        EXPECT_FALSE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_FALSE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(0u, sl.length());

        // activate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a true -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);

        // should now be able to select the items in the reference file
        EXPECT_TRUE(bool(sl.add("cube:pCube1")));
        EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();

        // activate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a false -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);

        // should now be able to select the items in the reference file
        EXPECT_FALSE(bool(sl.add("cube:pCube1")));
        EXPECT_FALSE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_FALSE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(0u, sl.length());

        // activate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a true -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);

        // should now be able to select the items in the reference file
        EXPECT_TRUE(bool(sl.add("cube:pCube1")));
        EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();
    }

    MFileIO::newFile(true);

    {
        MString    shapeName;
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
        shapeName = fn.name();

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        // stage should be valid
        EXPECT_TRUE(stage);

        // should be able to select the items in the reference file
        MSelectionList sl;
        EXPECT_TRUE(bool(sl.add("cube:pCube1")));
        EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();

        // activate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a false -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);

        EXPECT_FALSE(bool(sl.add("cube:pCube1")));
        EXPECT_FALSE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_FALSE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(0u, sl.length());

        // activate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a true -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);

        // should now be able to select the items in the reference file
        EXPECT_TRUE(bool(sl.add("cube:pCube1")));
        EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();

        // activate the prim
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a false -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);

        // should now be able to select the items in the reference file
        EXPECT_FALSE(bool(sl.add("cube:pCube1")));
        EXPECT_FALSE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_FALSE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(0u, sl.length());
    }

    MFileIO::newFile(true);

    {
        MString    shapeName;
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
        shapeName = fn.name();

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path3.c_str());

        auto stage = proxy->getUsdStage();

        // stage should be valid
        EXPECT_TRUE(stage);

        // the sphere variant is the default, so that should exist in the scene
        MSelectionList sl;
        EXPECT_TRUE(bool(sl.add("dave:pSphere1")));
        EXPECT_TRUE(bool(sl.add("dave:pSphereShape1")));
        EXPECT_TRUE(bool(sl.add("dave:polySphere1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();

        UsdPrim prim = stage->GetPrimAtPath(SdfPath("/root"));
        if (prim) {
            UsdVariantSet actualSet = prim.GetVariantSet("rig_technical");
            if (actualSet) {
                // should be able to set the variant to a cube
                EXPECT_TRUE(actualSet.SetVariantSelection("cube"));

                // sphere should not be there, but the cube should be
                EXPECT_FALSE(bool(sl.add("dave:pSphere1")));
                EXPECT_FALSE(bool(sl.add("dave:pSphereShape1")));
                EXPECT_FALSE(bool(sl.add("dave:polySphere1")));
                EXPECT_TRUE(bool(sl.add("dave:pCube1")));
                EXPECT_TRUE(bool(sl.add("dave:pCubeShape1")));
                EXPECT_TRUE(bool(sl.add("dave:polyCube1")));
                EXPECT_EQ(3u, sl.length());
                sl.clear();

                // should be able to set the variant back to a sphere
                EXPECT_TRUE(actualSet.SetVariantSelection("sphere"));

                // sphere should be there, but the cube should not be
                EXPECT_TRUE(bool(sl.add("dave:pSphere1")));
                EXPECT_TRUE(bool(sl.add("dave:pSphereShape1")));
                EXPECT_TRUE(bool(sl.add("dave:polySphere1")));
                EXPECT_FALSE(bool(sl.add("dave:pCube1")));
                EXPECT_FALSE(bool(sl.add("dave:pCubeShape1")));
                EXPECT_FALSE(bool(sl.add("dave:polyCube1")));
                EXPECT_EQ(3u, sl.length());
                sl.clear();

                // should be able to set the variant back to a cube
                EXPECT_TRUE(actualSet.SetVariantSelection("cube"));

                // sphere should not be there, but the cube should be
                EXPECT_FALSE(bool(sl.add("dave:pSphere1")));
                EXPECT_FALSE(bool(sl.add("dave:pSphereShape1")));
                EXPECT_FALSE(bool(sl.add("dave:polySphere1")));
                EXPECT_TRUE(bool(sl.add("dave:pCube1")));
                EXPECT_TRUE(bool(sl.add("dave:pCubeShape1")));
                EXPECT_TRUE(bool(sl.add("dave:polyCube1")));
                EXPECT_EQ(3u, sl.length());
                sl.clear();

                // should be able to set the variant to cube with ns fred
                // Make sure the same prim will only bring in one copy of a reference.
                // "cube" -> "fredcube: same filepath, but new ns
                EXPECT_TRUE(actualSet.SetVariantSelection("fredcube"));

                // cube ref should be loaded under ns fred
                EXPECT_FALSE(bool(sl.add("dave:pSphere1")));
                EXPECT_FALSE(bool(sl.add("dave:pSphereShape1")));
                EXPECT_FALSE(bool(sl.add("dave:polySphere1")));
                EXPECT_FALSE(bool(sl.add("dave:pCube1")));
                EXPECT_FALSE(bool(sl.add("dave:pCubeShape1")));
                EXPECT_FALSE(bool(sl.add("dave:polyCube1")));
                EXPECT_TRUE(bool(sl.add("fred:pCube1")));
                EXPECT_TRUE(bool(sl.add("fred:pCubeShape1")));
                EXPECT_TRUE(bool(sl.add("fred:polyCube1")));
                sl.clear();

                // should be able to set the variant back to a cube with ns dave
                EXPECT_TRUE(actualSet.SetVariantSelection("cube"));

                // sphere should not be there, but the cube should be
                EXPECT_FALSE(bool(sl.add("dave:pSphere1")));
                EXPECT_FALSE(bool(sl.add("dave:pSphereShape1")));
                EXPECT_FALSE(bool(sl.add("dave:polySphere1")));
                EXPECT_TRUE(bool(sl.add("dave:pCube1")));
                EXPECT_TRUE(bool(sl.add("dave:pCubeShape1")));
                EXPECT_TRUE(bool(sl.add("dave:polyCube1")));
                EXPECT_FALSE(bool(sl.add("fred:pCube1")));
                EXPECT_FALSE(bool(sl.add("fred:pCubeShape1")));
                EXPECT_FALSE(bool(sl.add("fred:polyCube1")));
                EXPECT_EQ(3u, sl.length());
                sl.clear();

                // should be able to set the variant back to a cached sphere
                EXPECT_TRUE(actualSet.SetVariantSelection("cache"));

                // no refs should be loaded anymore
                EXPECT_FALSE(bool(sl.add("dave:pSphere1")));
                EXPECT_FALSE(bool(sl.add("dave:pSphereShape1")));
                EXPECT_FALSE(bool(sl.add("dave:polySphere1")));
                EXPECT_FALSE(bool(sl.add("dave:pCube1")));
                EXPECT_FALSE(bool(sl.add("dave:pCubeShape1")));
                EXPECT_FALSE(bool(sl.add("dave:polyCube1")));
                EXPECT_FALSE(bool(sl.add("fred:pCube1")));
                EXPECT_FALSE(bool(sl.add("fred:pCubeShape1")));
                EXPECT_FALSE(bool(sl.add("fred:polyCube1")));
                EXPECT_EQ(0u, sl.length());

                // With any luck, the transform chain above the prim should have been removed
                // leaving us with no AL_usdmaya_transforms in the scene
                {
                    MItDependencyNodes iter(MFn::kPluginTransformNode);
                    EXPECT_TRUE(iter.isDone());
                }

                // Now when we set the variant back to a maya reference, we should be in a situation
                // where the transform chain has re-appeared, and the correct reference has been
                // imported into the scene
                EXPECT_TRUE(actualSet.SetVariantSelection("cube"));

                // only the cube should be loaded
                EXPECT_FALSE(bool(sl.add("dave:pSphere1")));
                EXPECT_FALSE(bool(sl.add("dave:pSphereShape1")));
                EXPECT_FALSE(bool(sl.add("dave:polySphere1")));
                EXPECT_TRUE(bool(sl.add("dave:pCube1")));
                EXPECT_TRUE(bool(sl.add("dave:pCubeShape1")));
                EXPECT_TRUE(bool(sl.add("dave:polyCube1")));
                EXPECT_FALSE(bool(sl.add("fred:pCube1")));
                EXPECT_FALSE(bool(sl.add("fred:pCubeShape1")));
                EXPECT_FALSE(bool(sl.add("fred:polyCube1")));
                EXPECT_EQ(3u, sl.length());
                sl.clear();

                // check to make sure the transform chain is back
                {
                    MItDependencyNodes iter(MFn::kPluginTransformNode);
                    EXPECT_FALSE(iter.isDone());
                }

                // import a reference, and make sure a new ref is created on a resync.
                EXPECT_TRUE(bool(sl.add("dave:pCube1")));
                EXPECT_FALSE(bool(sl.add("dave1:pCube1")));
                MString command;
                command = "file -importReference \"";
                command += temp_path_cube.asChar();
                command += "\";";
                MStatus status = MGlobal::executeCommand(command);
                EXPECT_EQ(MStatus(MS::kSuccess), status);
                // old reference is now imported
                EXPECT_TRUE(bool(sl.add("dave:pCube1")));
                EXPECT_FALSE(bool(sl.add("dave1:pCube1")));
                proxy->resync(SdfPath("/"));
                // old will still exist and a new reference has been created
                EXPECT_TRUE(bool(sl.add("dave:pCube1")));
                EXPECT_TRUE(bool(sl.add("dave1:pCube1")));
            }
        }
    }

    MFileIO::newFile(true);

    // * load an active reference
    // * deactivate it
    // * save the scene
    // * load the file back up
    // * activate the reference
    {
        MString shapeName;

        {
            MFnDagNode fn;
            MObject    xform = fn.create("transform");
            MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
            shapeName = fn.name();

            AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

            // force the stage to load
            proxy->filePathPlug().setString(temp_path.c_str());

            auto stage = proxy->getUsdStage();

            // stage should be valid
            EXPECT_TRUE(stage);

            // should not be able to select the items in the reference file
            MSelectionList sl;
            EXPECT_TRUE(bool(sl.add("cube:pCube1")));
            EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
            EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
            EXPECT_EQ(3u, sl.length());
            sl.clear();

            // activate the prim
            MGlobal::executeCommand(
                "AL_usdmaya_ActivatePrim -a false -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
                false,
                false);

            // should now be able to select the items in the reference file
            EXPECT_FALSE(bool(sl.add("cube:pCube1")));
            EXPECT_FALSE(bool(sl.add("cube:pCubeShape1")));
            EXPECT_FALSE(bool(sl.add("cube:polyCube1")));
            EXPECT_EQ(0u, sl.length());

            EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::saveAs(temp_ma_path3.c_str(), 0, true));

            MFileIO::newFile(true);
        }

        MFileIO::open(temp_ma_path3.c_str(), 0, true);

        MFnDagNode     fn;
        MSelectionList sl;
        sl.add(shapeName);
        MObject shape;
        sl.getDependNode(0, shape);
        sl.clear();

        EXPECT_EQ(MStatus(MS::kSuccess), fn.setObject(shape));

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
        EXPECT_TRUE(proxy != 0);

        EXPECT_FALSE(bool(sl.add("cube:pCube1")));
        EXPECT_FALSE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_FALSE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(0u, sl.length());

        // activate the prim, this should ensure the
        MGlobal::executeCommand(
            "AL_usdmaya_ActivatePrim -a true -pp \"/root/rig\" \"AL_usdmaya_ProxyShape1\"",
            false,
            false);

        EXPECT_TRUE(bool(sl.add("cube:pCube1")));
        EXPECT_TRUE(bool(sl.add("cube:pCubeShape1")));
        EXPECT_TRUE(bool(sl.add("cube:polyCube1")));
        EXPECT_EQ(3u, sl.length());
        sl.clear();
    }
}

TEST_F(ActiveInactive, variantChange)
{
    const std::string temp_path = buildTempPath("AL_USDMayaTests_variant.usda");

    // generate some data for the proxy shape
    {
        std::ofstream os(temp_path);
        os << g_variantSwitchPrimTypes;
    }

    MFileIO::newFile(true);
    MString shapeName;

    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
    shapeName = fn.name();

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());

    auto stage = proxy->getUsdStage();

    {
        // stage should be valid
        EXPECT_TRUE(stage);

        // should be composed of two layers
        SdfLayerHandle session = stage->GetSessionLayer();
        SdfLayerHandle root = stage->GetRootLayer();
        EXPECT_TRUE(session);
        EXPECT_TRUE(root);
    }
    // activate the prim or it wont be in the scene yet.
    MGlobal::executeCommand(
        "AL_usdmaya_ActivatePrim -a true -pp \"/root/switchable/top/cam\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        false);

    // camera variant is the default

    MSelectionList sl;
    EXPECT_TRUE(bool(sl.add("root")));
    EXPECT_TRUE(bool(sl.add("switchable")));
    EXPECT_TRUE(bool(sl.add("switchable|top")));
    EXPECT_TRUE(bool(sl.add("cam")));
    EXPECT_EQ(4u, sl.length());
    sl.clear();

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/root/switchable"));
    if (prim) {
        UsdVariantSet optionSet = prim.GetVariantSet("option");
        if (optionSet) {
            EXPECT_TRUE(optionSet.SetVariantSelection("mayaReference"));

            // make sure the translator was able to clear off transforms from past variant
            EXPECT_TRUE(bool(sl.add("root")));
            EXPECT_TRUE(bool(sl.add("switchable|top")));
            EXPECT_FALSE(bool(sl.add("cam")));
            EXPECT_TRUE(bool(sl.add("cam_ns:camera_rigg_top")));
            EXPECT_TRUE(bool(sl.add("cam_ns:camera1")));
            EXPECT_TRUE(bool(sl.add("cam_ns:cameraShape1")));
            EXPECT_EQ(5u, sl.length());
            sl.clear();

            // make sure we can switch back
            EXPECT_TRUE(optionSet.SetVariantSelection("camera"));

            EXPECT_TRUE(bool(sl.add("root")));
            EXPECT_TRUE(bool(sl.add("switchable")));
            EXPECT_TRUE(bool(sl.add("switchable|top")));
            EXPECT_TRUE(bool(sl.add("cam")));
            EXPECT_EQ(4u, sl.length());
            sl.clear();

            EXPECT_TRUE(optionSet.SetVariantSelection("no_translator"));

            // no translators so there should be no transforms in maya until they are selected
            EXPECT_FALSE(bool(sl.add("root")));
            EXPECT_FALSE(bool(sl.add("switchable|top")));
            EXPECT_FALSE(bool(sl.add("cam")));
            EXPECT_FALSE(bool(sl.add("cam_ns:camera_rigg_top")));
            EXPECT_FALSE(bool(sl.add("cam_ns:camera1")));
            EXPECT_FALSE(bool(sl.add("cam_ns:cameraShape1")));
            EXPECT_EQ(0u, sl.length());
            sl.clear();
        }
    }
}
