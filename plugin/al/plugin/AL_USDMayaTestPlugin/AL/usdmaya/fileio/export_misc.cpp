
//
// Copyright 2020 Animal Logic
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

#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

#include <string>

using AL::maya::test::buildTempPath;

TEST(export_misc, subdivision_scheme)
{
    auto geomType = TfType::Find<UsdGeomMesh>();
#if PXR_VERSION > 2002
    auto geomTypeToken = UsdSchemaRegistry::GetInstance().GetConcreteSchemaTypeName(geomType);
    auto primDef = UsdSchemaRegistry::GetInstance().FindConcretePrimDefinition(geomTypeToken);
    ASSERT_TRUE(primDef);
    auto attrSpec = primDef->GetSchemaAttributeSpec(UsdGeomTokens->subdivisionScheme);
#else
    auto attrSpec = UsdSchemaRegistry::GetAttributeDefinition(
        TfToken(geomType.GetTypeName()), UsdGeomTokens->subdivisionScheme);
#endif
    auto                   defaultToken = attrSpec->GetDefaultValue().UncheckedGet<TfToken>();
    std::map<TfToken, int> subdSchemeMap = { { defaultToken, 0 }, // No value should be authored
                                             { UsdGeomTokens->catmullClark, 1 },
                                             { UsdGeomTokens->none, 2 },
                                             { UsdGeomTokens->loop, 3 },
                                             { UsdGeomTokens->bilinear, 4 } };
    MFileIO::newFile(true);
    MGlobal::executeCommand("polySphere -n testSphere;");
    const std::string temp_path = buildTempPath("AL_USDMayaTests_subdivision_scheme.usda");

    auto buildCommand = [&temp_path](int subdScheme) {
        const std::string strCommand = "file -force -options "
                                       "\"Filter_Sample=0;"
                                       "Subdivision_scheme=^1s;\""
                                       "-typ \"AL usdmaya export\" -pr -es \" ^2s\";";
        MString command;
        command.format(
            strCommand.c_str(),
            MString(std::to_string(subdScheme).c_str()),
            MString(temp_path.c_str()));
        return command;
    };

    auto checkAttribute = [&temp_path](TfToken subdToken, bool hasAuthored) {
        UsdStageRefPtr stage = UsdStage::Open(temp_path);
        ASSERT_TRUE(stage);

        stage->Reload();
        UsdPrim     prim = stage->GetPrimAtPath(SdfPath("/testSphere"));
        UsdGeomMesh mesh(prim);

        UsdAttribute subdSchemeAttr = mesh.GetSubdivisionSchemeAttr();
        ASSERT_TRUE(subdSchemeAttr);
        if (hasAuthored) {
            EXPECT_TRUE(subdSchemeAttr.HasAuthoredValue());
        } else {
            EXPECT_FALSE(subdSchemeAttr.HasAuthoredValue());
        }

        TfToken subdScheme;
        subdSchemeAttr.Get(&subdScheme);
        EXPECT_EQ(subdToken, subdScheme);
    };

    for (const auto& kv : subdSchemeMap) {
        MGlobal::executeCommand(buildCommand(kv.second));
        // kv.second > 0 means no value should have been authored
        checkAttribute(kv.first, kv.second);
    };
}
