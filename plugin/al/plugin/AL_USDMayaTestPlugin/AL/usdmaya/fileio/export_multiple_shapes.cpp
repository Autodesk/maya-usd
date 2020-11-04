#include "test_usdmaya.h"

#include <pxr/usd/usdGeom/camera.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

using AL::maya::test::buildTempPath;

static const char* const generateCamera = R"(
{
string $cam[] = `camera`;
rename $cam[0] "foofoo";
string $plane = `createNode "imagePlane" -p "foofoo" -n "foofooImagePlane"`;
}
)";

static const TfToken xformPrimTypeName("Xform");

static const MString outputCommandTemplate
    = "select -r \"foofoo\";"
      "file -force -options "
      "\"Dynamic_Attributes=1;"
      "Meshes=1;"
      "Nurbs_Curves=1;"
      "Duplicate_Instances=1;"
      "Merge_Transforms=^1s;"
      "Animation=1;"
      "Use_Timeline_Range=0;"
      "Frame_Min=1;"
      "Frame_Max=50;"
      "Filter_Sample=0;\" -typ \"AL usdmaya export\" -pr -es \"^2s\";";

TEST(export_merged, merged_multiple_shape)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(generateCamera);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_mergedMultipleShape.usda");
    MString           command;
    command.format(outputCommandTemplate, "1", temp_path.c_str());

    MGlobal::executeCommand(command);

    UsdStageRefPtr stage = UsdStage::Open(temp_path);
    EXPECT_TRUE(stage);

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/foofoo"));
    EXPECT_TRUE(prim.IsValid());
    // exported prim's exact prim type depends on which translator is available, but it is
    // guaranteed to inherit from UsdGeomCamera
    EXPECT_TRUE(prim.IsA<UsdGeomCamera>());

    prim = stage->GetPrimAtPath(SdfPath("/foofoo/foofooShape"));
    EXPECT_FALSE(prim.IsValid());
}

TEST(export_unmerged, unmerged_multiple_shape)
{

    MFileIO::newFile(true);
    MGlobal::executeCommand(generateCamera);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_unmergedMultipleShape.usda");
    MString           command;
    command.format(outputCommandTemplate, "0", temp_path.c_str());

    MGlobal::executeCommand(command);

    UsdStageRefPtr stage = UsdStage::Open(temp_path);
    EXPECT_TRUE(stage);

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/foofoo"));
    EXPECT_TRUE(prim.IsValid());
    EXPECT_TRUE(prim.GetTypeName() == xformPrimTypeName);

    prim = stage->GetPrimAtPath(SdfPath("/foofoo/foofooShape"));
    EXPECT_TRUE(prim.IsValid());
    EXPECT_TRUE(prim.IsA<UsdGeomCamera>());

    prim = stage->GetPrimAtPath(SdfPath("/foofoo/foofooImagePlane"));
    EXPECT_TRUE(prim.IsValid());
    // We can not expect the type of image plane here since current it is Xform and is liable to
    // change in future.
}
