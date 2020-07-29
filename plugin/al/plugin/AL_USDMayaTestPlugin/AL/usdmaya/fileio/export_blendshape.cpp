
#include "test_usdmaya.h"

#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

using AL::maya::test::buildTempPath;

static const char* const g_nonAnimatedMesh = R"(
{
polyCube -w 1 -h 1 -d 1 -sx 1 -sy 1 -sz 1 -ax 0 1 0 -cuv 2 -ch 1 -name "baseCube";
polyCube -w 1 -h 1 -d 1 -sx 1 -sy 1 -sz 1 -ax 0 1 0 -cuv 2 -ch 1 -name "blendshape";
select -r "blendshape.vtx[2]";
move -r -0.2 0.2 0.2;
select -r "blendshape";
select -add "baseCube";
$deformer = `blendShape`;
}
)";

static const char* const g_animatedMesh = R"(
{
polyCube -w 1 -h 1 -d 1 -sx 1 -sy 1 -sz 1 -ax 0 1 0 -cuv 2 -ch 1 -name "baseCube";
polyCube -w 1 -h 1 -d 1 -sx 1 -sy 1 -sz 1 -ax 0 1 0 -cuv 2 -ch 1 -name "blendshape";
select -r "blendshape.vtx[2]";
move -r -0.2 0.2 0.2;
select -r "blendshape";
select -add "baseCube";
$deformer = `blendShape`;
currentTime 1;
setKeyframe ($deformer[0] + "." + "blendshape" );
currentTime 50;
setAttr ($deformer[0] + "." + "blendshape" ) 1;
setKeyframe ($deformer[0] + "." + "blendshape" );
}
)";

static const char* const g_timeBoundAnimatedMesh = R"(
{
polyCube -w 1 -h 1 -d 1 -sx 1 -sy 1 -sz 1 -ax 0 1 0 -cuv 2 -ch 1 -name "baseCube";
polyCube -w 1 -h 1 -d 1 -sx 1 -sy 1 -sz 1 -ax 0 1 0 -cuv 2 -ch 1 -name "blendshape";
select -r "blendshape.vtx[2]";
move -r -0.2 0.2 0.2;
select -r "blendshape";
select -add "baseCube";
$deformer = `blendShape`;
setAttr ($deformer[0] + "." + "blendshape" ) 1;
connectAttr -f time1.outTime ($deformer[0] + ".envelope");
}
)";

TEST(export_blendshape, non_animated_mesh)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(g_nonAnimatedMesh);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_blendshape.usda");

    MString command = "select -r \"baseCube\";"
                      "file -force -options "
                      "\"Dynamic_Attributes=1;"
                      "Meshes=1;"
                      "Nurbs_Curves=1;"
                      "Duplicate_Instances=1;"
                      "Merge_Transforms=1;"
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

    UsdPrim     prim = stage->GetPrimAtPath(SdfPath("/baseCube"));
    UsdGeomMesh mesh(prim);

    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    ASSERT_TRUE(pointsAttr);
    size_t size = pointsAttr.GetNumTimeSamples();
    EXPECT_EQ(0u, size);
}

TEST(export_blendshape, animated_mesh)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(g_animatedMesh);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_anim_blendshape.usda");

    MString command = "select -r \"baseCube\";"
                      "file -force -options "
                      "\"Dynamic_Attributes=1;"
                      "Meshes=1;"
                      "Nurbs_Curves=1;"
                      "Duplicate_Instances=1;"
                      "Merge_Transforms=1;"
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

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/baseCube"));
    ASSERT_TRUE(prim.IsA<UsdGeomMesh>());
    UsdGeomMesh mesh(prim);

    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    ASSERT_TRUE(pointsAttr);
    size_t size = pointsAttr.GetNumTimeSamples();
    EXPECT_EQ(50u, size);
}

TEST(export_blendshape, time_bound_animated_mesh)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(g_timeBoundAnimatedMesh);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_time_bound_anim_blendshape.usda");

    MString command = "select -r \"baseCube\";"
                      "file -force -options "
                      "\"Dynamic_Attributes=1;"
                      "Meshes=1;"
                      "Nurbs_Curves=1;"
                      "Duplicate_Instances=1;"
                      "Merge_Transforms=1;"
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

    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/baseCube"));
    ASSERT_TRUE(prim.IsA<UsdGeomMesh>());
    UsdGeomMesh mesh(prim);

    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    ASSERT_TRUE(pointsAttr);
    size_t size = pointsAttr.GetNumTimeSamples();
    EXPECT_EQ(50u, size);
}
