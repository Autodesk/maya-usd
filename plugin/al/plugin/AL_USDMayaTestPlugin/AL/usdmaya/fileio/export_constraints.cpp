#include "test_usdmaya.h"

#include <pxr/usd/usdGeom/xform.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

using AL::maya::test::buildTempPath;

static const char* const g_constraints = R"(
{
$s = `polyCylinder -r 1 -h 4 -sx 20 -sy 20 -sz 1 -ax 0 1 0 -rcp 0 -cuv 3 -ch 1`;
$j1 = `joint -p 0 -2 0`;
$j2 = `joint -p 0 0 0`;
joint -e -zso -oj xyz -sao yup $j2;
$j3 = `joint -p 0 2 0`;
joint -e -zso -oj xyz -sao yup $j3;
select -tgl $s;
newSkinCluster "-bindMethod 0 -normalizeWeights 1 -weightDistribution 0 -mi 5 -omi true -dr 4 -rui true,multipleBindPose,1";

$loc8 = `spaceLocator -n "normalLoc"`;
move 1 1 0;
select -r $s;
select -add $loc8;
normalConstraint;

currentTime 1;
setKeyframe ($loc8[0] + ".tx");
setKeyframe ($j2 + ".rz");
setKeyframe ($j2 + ".sz");
currentTime 50;
setAttr ($loc8[0] + ".tx") 2;
setAttr ($j2 + ".rz") 90;
setAttr ($j2 + ".sz") 3;
setKeyframe ($loc8[0] + ".tx");
setKeyframe ($j2 + ".rz");
setKeyframe ($j2 + ".sz");
currentTime 1;

$loc1 = `spaceLocator -n "parentLoc"`;
select -r $j3;
select -add $loc1;
parentConstraint;

$loc2 = `spaceLocator -n "orientLoc"`;
select -r $j3;
select -add $loc2;
orientConstraint;

$loc3 = `spaceLocator -n "pointLoc"`;
select -r $j3;
select -add $loc3;
pointConstraint;

$loc4 = `spaceLocator -n "scaleLoc"`;
select -r $j3;
select -add $loc4;
scaleConstraint;

$loc5 = `spaceLocator -n "aimLoc"`;
select -r $j3;
select -add $loc5;
aimConstraint;

$loc6 = `spaceLocator -n "geomLoc"`;
move -r 0 1 -1;
select -r $s;
select -add $loc6;
geometryConstraint;

$loc7 = `spaceLocator -n "pointOnPolyLoc"`;
select -r ("pCylinder1" + ".f[131]");
select -add $loc7;
$constraint = `pointOnPolyConstraint -o 0 0 0`;
setAttr ($constraint[0] + ".pCylinder1V0") 0.5;
setAttr ($constraint[0] + ".pCylinder1W0") 0.5;
setAttr ($constraint[0] + ".pCylinder1U0") 0.5;


$curve = `curve -d 3 -p 0 2.267605 1.112862 -p 0 -0.0299156 1.818871 -p 0 -1.07098 1.400052 -p 0 -1.788955 1.435951 -p 0 -2.12401 1.771006 -p 0 -2.710357 0.993199 -k 0 -k 0 -k 0 -k 1 -k 2 -k 3 -k 3 -k 3`;

select -r $j3 $j2 $j1;
select -add $curve;
newSkinCluster "-bindMethod 0 -normalizeWeights 1 -weightDistribution 0 -mi 5 -omi true -dr 4 -rui true,multipleBindPose,1";
$loc9 = `spaceLocator -n "tangentLoc"`;
select -r $curve;
select -add $loc9;
tangentConstraint;
}
)";

TEST(export_constraint, constraints)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(g_constraints);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_constraints.usda");

    MString command
        = "select -r \"polyCylinder1\" \"parentLoc\" \"orientLoc\" \"pointLoc\" \"scaleLoc\" "
          "\"aimLoc\" \"geomLoc\" \"pointOnPolyLoc\" \"normalLoc\" \"tangentLoc\";"
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

    const char* const paths[]
        = { "/parentLoc", "/orientLoc",      "/pointLoc",  "/scaleLoc",   "/aimLoc",
            "/geomLoc",   "/pointOnPolyLoc", "/normalLoc", "/tangentLoc", 0 };

    const char* const* iter = paths;
    while (*iter) {
        UsdPrim      prim = stage->GetPrimAtPath(SdfPath(*iter));
        UsdGeomXform transform(prim);

        bool                        resetsXformStack;
        std::vector<UsdGeomXformOp> ops = transform.GetOrderedXformOps(&resetsXformStack);
        EXPECT_FALSE(ops.empty());
        for (auto op : ops) {
            auto attr = op.GetAttr();
            EXPECT_EQ(50u, attr.GetNumTimeSamples());
        }
        ++iter;
    }
}
