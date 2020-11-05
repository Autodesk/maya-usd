//
// Copyright 2019 Animal Logic
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
#include "test_usdmaya.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>

using AL::maya::test::buildTempPath;

// export a poly cube with all translators disabled, and no
TEST(PluginTranslators, activeInactive1)
{
    MFileIO::newFile(true);
    const char* const create_cmd = "polyCube;";
    std::string       export_cmd = R"(
  file -force -options "Merge_Transforms=1;Animation=0;Export_At_Which_Time=2;Export_In_World_Space=1;Activate_all_Plugin_Translators=0;Active_Translator_List=;Meshes=1;Mesh_Face_Connects=1;Mesh_Points=1;Mesh_UV_Only=0;" -typ "AL usdmaya export" -pr -es
  )";
    auto              path = buildTempPath("AL_USDMayaTests_ativeInactiveTranslators1.usda");
    export_cmd += "\"";
    export_cmd += path;
    export_cmd += "\"";

    MGlobal::executeCommand(create_cmd);
    MGlobal::executeCommand(export_cmd.c_str());

    UsdStageRefPtr stage = UsdStage::Open(path);
    EXPECT_TRUE(stage);

    // resulting prim should exist (as a transform), but will not be a mesh.
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    EXPECT_TRUE(prim.IsValid());
    EXPECT_FALSE(prim.IsA<UsdGeomMesh>());
}

// export a polyCube with all translators disabled, but with the mesh translator enabled.
TEST(PluginTranslators, activeInactive2)
{
    MFileIO::newFile(true);
    const char* const create_cmd = "polyCube;";
    std::string       export_cmd = R"(
  file -force -options "Merge_Transforms=1;Animation=0;Export_At_Which_Time=2;Export_In_World_Space=1;Activate_all_Plugin_Translators=0;Active_Translator_List=UsdGeomMesh;Meshes=1;Mesh_Face_Connects=1;Mesh_Points=1;Mesh_UV_Only=0;" -typ "AL usdmaya export" -pr -es
  )";
    auto              path = buildTempPath("AL_USDMayaTests_ativeInactiveTranslators2.usda");
    export_cmd += "\"";
    export_cmd += path;
    export_cmd += "\"";

    MGlobal::executeCommand(create_cmd);
    MGlobal::executeCommand(export_cmd.c_str());

    UsdStageRefPtr stage = UsdStage::Open(path);
    EXPECT_TRUE(stage);

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    EXPECT_TRUE(prim.IsValid());
    EXPECT_TRUE(prim.IsA<UsdGeomMesh>());
}

// export a polyCube with all translators enabled, but with the mesh translator disabled.
TEST(PluginTranslators, activeInactive3)
{
    MFileIO::newFile(true);
    const char* const create_cmd = "polyCube;";
    std::string       export_cmd = R"(
  file -force -options "Merge_Transforms=1;Animation=0;Export_At_Which_Time=2;Export_In_World_Space=1;Activate_all_Plugin_Translators=1;Active_Translator_List=;Inactive_Translator_List=;Meshes=1;Mesh_Face_Connects=1;Mesh_Points=1;Mesh_UV_Only=0;" -typ "AL usdmaya export" -pr -es
  )";
    auto              path = buildTempPath("AL_USDMayaTests_ativeInactiveTranslators3.usda");
    export_cmd += "\"";
    export_cmd += path;
    export_cmd += "\"";

    MGlobal::executeCommand(create_cmd);
    MGlobal::executeCommand(export_cmd.c_str());

    UsdStageRefPtr stage = UsdStage::Open(path);
    EXPECT_TRUE(stage);

    // resulting prim should exist (as a transform), but will not be a mesh.
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    EXPECT_TRUE(prim.IsValid());
    EXPECT_TRUE(prim.IsA<UsdGeomMesh>());
}

// export a poly cube with all translators enabled, and no optionally disabled translators
TEST(PluginTranslators, activeInactive4)
{
    MFileIO::newFile(true);
    const char* const create_cmd = "polyCube;";
    std::string       export_cmd = R"(
  file -force -options "Merge_Transforms=1;Animation=0;Export_At_Which_Time=2;Export_In_World_Space=1;Activate_all_Plugin_Translators=1;Active_Translator_List=;Inactive_Translator_List=UsdGeomMesh;Meshes=1;Mesh_Face_Connects=1;Mesh_Points=1;Mesh_UV_Only=0;" -typ "AL usdmaya export" -pr -es
  )";
    auto              path = buildTempPath("AL_USDMayaTests_ativeInactiveTranslators4.usda");
    export_cmd += "\"";
    export_cmd += path;
    export_cmd += "\"";

    MGlobal::executeCommand(create_cmd);
    MGlobal::executeCommand(export_cmd.c_str());

    UsdStageRefPtr stage = UsdStage::Open(path);
    EXPECT_TRUE(stage);

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    EXPECT_TRUE(prim.IsValid());
    EXPECT_FALSE(prim.IsA<UsdGeomMesh>());
}
