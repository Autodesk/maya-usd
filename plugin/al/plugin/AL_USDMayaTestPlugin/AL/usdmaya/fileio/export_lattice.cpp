#include "test_usdmaya.h"

#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

using AL::maya::test::buildTempPath;

static const char* const g_ffd = R"(
{
  $c = `polyCylinder -r 1 -h 4 -sx 20 -sy 20 -sz 1 -ax 0 1 0 -rcp 0 -cuv 3 -ch 1`;
  $l = `lattice -divisions 5 5 5 -objectCentered true -ldv 2 2 2`;
}
)";

static const char* const g_ffd_animated = R"(
{
$c = `polyCylinder -r 1 -h 4 -sx 20 -sy 20 -sz 1 -ax 0 1 0 -rcp 0 -cuv 3 -ch 1`;
$l = `lattice -divisions 5 5 5 -objectCentered true -ldv 2 2 2`;
select -r ($l[1] + ".pt[0][4][4]");
setKeyframe -breakdown 0 -hierarchy none -controlPoints 0 -shape 0 ($l[1] + ".pt[0][4][4]");
currentTime 50 ;
move -r -1.403299 1.128142 0.549356 ;
setKeyframe -breakdown 0 -hierarchy none -controlPoints 0 -shape 0 ($l[1] + ".pt[0][4][4]");
}
)";

TEST(export_ffd, nonanimated)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(g_ffd);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_ffd.usda");

    MString command = "select -r \"pCylinder1\";"
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

    {
        UsdPrim     prim = stage->GetPrimAtPath(SdfPath("/pCylinder1"));
        UsdGeomMesh mesh(prim);

        UsdAttribute pointsAttr = mesh.GetPointsAttr();
        size_t       size = pointsAttr.GetNumTimeSamples();
        EXPECT_EQ(0u, size);
    }
}

TEST(export_ffd, animated)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(g_ffd_animated);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_ffd_animated.usda");

    MString command = "select -r \"pCylinder1\";"
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

    {
        UsdPrim     prim = stage->GetPrimAtPath(SdfPath("/pCylinder1"));
        UsdGeomMesh mesh(prim);

        UsdAttribute pointsAttr = mesh.GetPointsAttr();
        size_t       size = pointsAttr.GetNumTimeSamples();
        EXPECT_EQ(50u, size);
    }
}
