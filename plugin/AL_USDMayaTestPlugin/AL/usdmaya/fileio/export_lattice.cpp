#include <maya/MGlobal.h>
#include <maya/MFileIO.h>

#include "test_usdmaya.h"

#include "pxr/usd/usdGeom/mesh.h"

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

  const char* command =
  "select -r \"pCylinder1\";"
  "file -force -options "
  "\"Dynamic_Attributes=1;"
  "Meshes=1;"
  "Nurbs_Curves=1;"
  "Duplicate_Instances=1;"
  "Use_Animal_Schema=1;"
  "Merge_Transforms=1;"
  "Animation=1;"
  "Use_Timeline_Range=0;"
  "Frame_Min=1;"
  "Frame_Max=50;"
  "Filter_Sample=0;\" -typ \"AL usdmaya export\" -pr -es \"/tmp/AL_USDMayaTests_ffd.usda\";";

  MGlobal::executeCommand(command);

  UsdStageRefPtr stage = UsdStage::Open("/tmp/AL_USDMayaTests_ffd.usda");
  EXPECT_TRUE(stage);

  {
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCylinder1"));
    UsdGeomMesh mesh(prim);

    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    size_t size = pointsAttr.GetNumTimeSamples();
    EXPECT_EQ(0, size);
  }
}

TEST(export_ffd, animated)
{
  MFileIO::newFile(true);
  MGlobal::executeCommand(g_ffd_animated);

  const char* command =
  "select -r \"pCylinder1\";"
  "file -force -options "
  "\"Dynamic_Attributes=1;"
  "Meshes=1;"
  "Nurbs_Curves=1;"
  "Duplicate_Instances=1;"
  "Use_Animal_Schema=1;"
  "Merge_Transforms=1;"
  "Animation=1;"
  "Use_Timeline_Range=0;"
  "Frame_Min=1;"
  "Frame_Max=50;"
  "Filter_Sample=0;\" -typ \"AL usdmaya export\" -pr -es \"/tmp/AL_USDMayaTests_ffd_animated.usda\";";

  MGlobal::executeCommand(command);

  UsdStageRefPtr stage = UsdStage::Open("/tmp/AL_USDMayaTests_ffd_animated.usda");
  EXPECT_TRUE(stage);

  {
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCylinder1"));
    UsdGeomMesh mesh(prim);

    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    size_t size = pointsAttr.GetNumTimeSamples();
    EXPECT_EQ(50, size);
  }
}

