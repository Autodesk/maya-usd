#include "test_usdmaya.h"

#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

using AL::maya::test::buildTempPath;

static const char* const g_nonlinear = R"(
{
polyCylinder -r 1 -h 4 -sx 20 -sy 20 -sz 1 -ax 0 1 0 -rcp 0 -cuv 3 -ch 1;
$nl = `nonLinear -type bend  -lowBound -1 -highBound 1 -curvature 0`;
}
)";

static const char* const g_nonlinear_animated = R"(
{
polyCylinder -r 1 -h 4 -sx 20 -sy 20 -sz 1 -ax 0 1 0 -rcp 0 -cuv 3 -ch 1;
$nl = `nonLinear -type bend  -lowBound -1 -highBound 1 -curvature 0`;
currentTime 1;
setKeyframe ($nl[0] + ".cur");
currentTime 50;
setAttr ($nl[0] + ".cur") 25;
setKeyframe ($nl[0] + ".cur");
}
)";

TEST(export_nonlinear, nonanimated)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(g_nonlinear);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_nonlinear.usda");

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

TEST(export_nonlinear, animated)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(g_nonlinear_animated);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_nonlinear_animated.usda");

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
