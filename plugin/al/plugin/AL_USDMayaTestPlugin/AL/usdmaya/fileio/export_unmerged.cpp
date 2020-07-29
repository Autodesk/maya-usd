#include "AL/usdmaya/Metadata.h"
#include "test_usdmaya.h"

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

using AL::maya::test::buildTempPath;

static const char* const generateSphere = R"(
{
polySphere -r 1 -sx 20 -sy 20 -ax 0 1 0 -cuv 2 -ch 1;
rename "pSphereShape1" "foofoo";
}
)";

TEST(export_unmerged, unmerged_metadata_parent_transform_exists)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(generateSphere);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_sphere.usda");

    MString command = "select -r \"pSphere1\";"
                      "file -force -options "
                      "\"Dynamic_Attributes=1;"
                      "Meshes=1;"
                      "Nurbs_Curves=1;"
                      "Duplicate_Instances=1;"
                      "Merge_Transforms=0;"
                      "Animation=1;"
                      "Use_Timeline_Range=0;"
                      "Frame_Min=1;"
                      "Frame_Max=50;"
                      "Filter_Sample=0;\" -typ \"AL usdmaya export\" -pr -es \"";
    command += temp_path.c_str();
    command += "\";";

    MGlobal::executeCommand(command);

    UsdStageRefPtr stage = UsdStage::Open(temp_path);
    EXPECT_TRUE(stage);

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pSphere1"));
    EXPECT_TRUE(prim.IsValid());
}

TEST(export_unmerged, unmerged_metadata_shape_name_preserved)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(generateSphere);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_sphere.usda");

    MString command = "select -r \"pSphere1\";"
                      "file -force -options "
                      "\"Dynamic_Attributes=1;"
                      "Meshes=1;"
                      "Nurbs_Curves=1;"
                      "Duplicate_Instances=1;"
                      "Merge_Transforms=0;"
                      "Animation=1;"
                      "Use_Timeline_Range=0;"
                      "Frame_Min=1;"
                      "Filter_Sample=0;\" -typ \"AL usdmaya export\" -pr -es \"";
    command += temp_path.c_str();
    command += "\";";

    MGlobal::executeCommand(command);

    UsdStageRefPtr stage = UsdStage::Open(temp_path);
    EXPECT_TRUE(stage);

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pSphere1/foofoo"));
    EXPECT_TRUE(prim.IsValid());
}

TEST(export_unmerged, unmerged_metadata_correctly_labelled_on_parent_transform)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(generateSphere);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_sphere.usda");

    MString command = "select -r \"pSphere1\";"
                      "file -force -options "
                      "\"Dynamic_Attributes=1;"
                      "Meshes=1;"
                      "Nurbs_Curves=1;"
                      "Duplicate_Instances=1;"
                      "Merge_Transforms=0;"
                      "Animation=1;"
                      "Use_Timeline_Range=0;"
                      "Frame_Min=1;"
                      "Frame_Max=50;"
                      "Filter_Sample=0;\" -typ \"AL usdmaya export\" -pr -es \"";
    command += temp_path.c_str();
    command += "\";";

    MGlobal::executeCommand(command);

    UsdStageRefPtr stage = UsdStage::Open(temp_path);
    EXPECT_TRUE(stage);

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pSphere1"));
    EXPECT_TRUE(prim.IsValid());
    TfToken val;
    EXPECT_TRUE(prim.GetMetadata(AL::usdmaya::Metadata::mergedTransform, &val));
    EXPECT_TRUE(val == AL::usdmaya::Metadata::unmerged);

    prim = stage->GetPrimAtPath(SdfPath("/pSphere1/foofoo"));
    EXPECT_TRUE(prim.IsValid());
    EXPECT_FALSE(prim.GetMetadata(AL::usdmaya::Metadata::mergedTransform, &val));
}
