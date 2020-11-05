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

#include "AL/maya/utils/Utils.h"
#include "test_usdmaya.h"

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>

using AL::maya::test::buildTempPath;

TEST(ExportCommands, exportUVOnly)
{
    MFileIO::newFile(true);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_exportUVOnly.usda");
    MGlobal::executeCommand(
        MString("createNode transform -n geo;polyCube -n cube -cuv 2;parent cube geo;select geo"),
        false,
        true);
    MString exportCmd;
    exportCmd.format(
        MString("AL_usdmaya_ExportCommand -f \"^1s\" -sl 1 -uvo 1 -luv 1"),
        AL::maya::utils::convert(temp_path));
    MGlobal::executeCommand(exportCmd, true);

    UsdStageRefPtr stage = UsdStage::Open(temp_path);
    UsdPrim        geoPrim = stage->GetPrimAtPath(SdfPath("/geo"));
    ASSERT_TRUE(geoPrim.IsValid());
    ASSERT_EQ(geoPrim.GetSpecifier(), SdfSpecifierOver);

    UsdPrim cubePrim = stage->GetPrimAtPath(SdfPath("/geo/cube"));
    ASSERT_TRUE(cubePrim.IsValid());
    ASSERT_EQ(cubePrim.GetSpecifier(), SdfSpecifierOver);

    const uint32_t uvSZ = 24;
    const GfVec2f  faceUVs[4]
        = { GfVec2f(0.0f, 0.0f), GfVec2f(1.0f, 0.0f), GfVec2f(1.0f, 1.0f), GfVec2f(0.0f, 1.0f) };

    UsdAttribute stAttr = cubePrim.GetAttribute(TfToken("primvars:st"));
    ASSERT_TRUE(stAttr.IsValid());
    VtArray<GfVec2f> uvs;
    stAttr.Get(&uvs);
    ASSERT_EQ(uvs.size(), uvSZ);

    UsdGeomPrimvar primVar(stAttr);
    ASSERT_TRUE(primVar.IsIndexed());
    VtIntArray indices;
    primVar.GetIndices(&indices);
    ASSERT_EQ(indices.size(), uvSZ);

    for (uint32_t i = 0; i < uvSZ; ++i) {
        const GfVec2f& uv = uvs[indices[i]];
        const GfVec2f& faceUV = faceUVs[i % 4];
        ASSERT_FLOAT_EQ(uv[0], faceUV[0]);
        ASSERT_FLOAT_EQ(uv[1], faceUV[1]);
    }
}

TEST(ExportCommands, exportUVS)
{
    MFileIO::newFile(true);

    const std::string temp_path_withUV = buildTempPath("AL_USDMayaTests_exportUVS.usda");
    MGlobal::executeCommand(
        MString("createNode transform -n geo;polyCube -n cube -cuv 2;parent cube geo;select geo"),
        false,
        true);
    MString exportCmd;
    exportCmd.format(
        MString("AL_usdmaya_ExportCommand -f \"^1s\" -sl 1 -uvs 1 -luv 1"),
        AL::maya::utils::convert(temp_path_withUV));
    MGlobal::executeCommand(exportCmd, true);

    UsdStageRefPtr stage = UsdStage::Open(temp_path_withUV);
    UsdPrim        geoPrim = stage->GetPrimAtPath(SdfPath("/geo"));
    ASSERT_TRUE(geoPrim.IsValid());
    ASSERT_EQ(geoPrim.GetSpecifier(), SdfSpecifierDef);

    UsdPrim cubePrim = stage->GetPrimAtPath(SdfPath("/geo/cube"));
    ASSERT_TRUE(cubePrim.IsValid());
    ASSERT_EQ(cubePrim.GetSpecifier(), SdfSpecifierDef);

    const uint32_t uvSZ = 24;
    const GfVec2f  faceUVs[4]
        = { GfVec2f(0.0f, 0.0f), GfVec2f(1.0f, 0.0f), GfVec2f(1.0f, 1.0f), GfVec2f(0.0f, 1.0f) };

    UsdAttribute stAttr = cubePrim.GetAttribute(TfToken("primvars:st"));
    ASSERT_TRUE(stAttr.IsValid());
    VtArray<GfVec2f> uvs;
    stAttr.Get(&uvs);
    ASSERT_EQ(uvs.size(), uvSZ);

    UsdGeomPrimvar primVar(stAttr);
    ASSERT_TRUE(primVar.IsIndexed());
    VtIntArray indices;
    primVar.GetIndices(&indices);
    ASSERT_EQ(indices.size(), uvSZ);

    for (uint32_t i = 0; i < uvSZ; ++i) {
        const GfVec2f& uv = uvs[indices[i]];
        const GfVec2f& faceUV = faceUVs[i % 4];
        ASSERT_FLOAT_EQ(uv[0], faceUV[0]);
        ASSERT_FLOAT_EQ(uv[1], faceUV[1]);
    }

    // Now testing export without UV:
    const std::string temp_path_withoutUV = buildTempPath("AL_USDMayaTests_noUVS.usda");
    exportCmd.format(
        MString("AL_usdmaya_ExportCommand -f \"^1s\" -sl 1 -uvs 0 -luv 1"),
        AL::maya::utils::convert(temp_path_withoutUV));
    MGlobal::executeCommand(exportCmd, true);

    stage = UsdStage::Open(temp_path_withoutUV);
    geoPrim = stage->GetPrimAtPath(SdfPath("/geo"));
    ASSERT_TRUE(geoPrim.IsValid());
    ASSERT_EQ(geoPrim.GetSpecifier(), SdfSpecifierDef);

    cubePrim = stage->GetPrimAtPath(SdfPath("/geo/cube"));
    ASSERT_TRUE(cubePrim.IsValid());
    ASSERT_EQ(cubePrim.GetSpecifier(), SdfSpecifierDef);
    stAttr = cubePrim.GetAttribute(TfToken("primvars:st"));
    ASSERT_FALSE(stAttr.IsValid());
}

TEST(ExportCommands, extensiveAnimationCheck)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(
        MString("createNode transform -n parent;polyCube -n child;parent child parent;"),
        false,
        true);
    MGlobal::executeCommand(
        MString("createNode transform -n master;connectAttr master.tx parent.tx;select child;"),
        false,
        true);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_extensiveAnimationCheck.usda");
    MString           exportCmd;

    auto expectAnimation = [temp_path](bool expectAnimation) {
        UsdStageRefPtr stage = UsdStage::Open(temp_path);
        EXPECT_TRUE(stage);

        UsdPrim prim = stage->GetPrimAtPath(SdfPath("/child"));
        EXPECT_TRUE(prim.IsValid());

        UsdGeomXform transform(prim);

        bool                        resetsXformStack;
        std::vector<UsdGeomXformOp> ops = transform.GetOrderedXformOps(&resetsXformStack);
        if (expectAnimation) {
            EXPECT_FALSE(ops.empty());
            for (auto op : ops) {
                auto attr = op.GetAttr();
                EXPECT_EQ(10u, attr.GetNumTimeSamples());
            }
        } else {
            EXPECT_TRUE(ops.empty());
        }
    };

    // Test default behavior:
    exportCmd.format(
        MString("AL_usdmaya_ExportCommand -f \"^1s\" -sl 1 -frameRange 1 10"),
        AL::maya::utils::convert(temp_path));
    MGlobal::executeCommand(exportCmd, true);
    expectAnimation(true);

    // Test turning off the extensiveAnimationCheck:
    exportCmd.format(
        MString("AL_usdmaya_ExportCommand -f \"^1s\" -sl 1 -extensiveAnimationCheck 0 -frameRange "
                "1 10"),
        AL::maya::utils::convert(temp_path));
    MGlobal::executeCommand(exportCmd, true);
    expectAnimation(false);
}
