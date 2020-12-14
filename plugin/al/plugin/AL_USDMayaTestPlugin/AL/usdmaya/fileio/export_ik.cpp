#include "AL/usdmaya/TransformOperation.h"
#include "test_usdmaya.h"

#include <pxr/usd/usdGeom/xform.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

using AL::maya::test::buildTempPath;

static const char* const g_ikChain = R"(
{
select -cl;
$j4 = `joint -p 0 2 -5`;
$j5 = `joint -p 0 0 -7`;
joint -e -zso -oj xyz -sao yup $j5;
$j6 = `joint -p 0 -2 -5`;
joint -e -zso -oj xyz -sao yup $j6;
$loc10 = `spaceLocator -n "poleLoc"`;
select -r $j4;
select -add $j6;
$ik = `ikHandle`;
select -r $loc10;
select -add $ik[0];
poleVectorConstraint -weight 1;

currentTime 1;
setAttr ($ik[0] + ".tx") -2;
setAttr ($loc10[0] + ".tx") 2;
setKeyframe ($ik[0] + ".tx");
setKeyframe ($loc10[0] + ".tx");
currentTime 50;
setAttr ($ik[0] + ".tx") 2;
setAttr ($loc10[0] + ".tx") -2;
setKeyframe ($ik[0] + ".tx");
setKeyframe ($loc10[0] + ".tx");
}
)";

TEST(export_ik, ikchain)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(g_ikChain);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_ikchain.usda");

    MString command = "select -r \"joint1\";"
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
        UsdPrim      prim = stage->GetPrimAtPath(SdfPath("/joint1"));
        UsdGeomXform joint1(prim);

        bool                        resetsXformStack;
        std::vector<UsdGeomXformOp> ops = joint1.GetOrderedXformOps(&resetsXformStack);
        EXPECT_FALSE(ops.empty());
        for (auto op : ops) {
            auto attr = op.GetAttr();
            if (AL::usdmaya::xformOpToEnum(op.GetBaseName()) == AL::usdmaya::kTranslate) {
                EXPECT_EQ(0u, attr.GetNumTimeSamples());
            } else {
                EXPECT_EQ(50u, attr.GetNumTimeSamples());
            }
        }
    }

    {
        UsdPrim      prim = stage->GetPrimAtPath(SdfPath("/joint1/joint2"));
        UsdGeomXform joint2(prim);

        bool                        resetsXformStack;
        std::vector<UsdGeomXformOp> ops = joint2.GetOrderedXformOps(&resetsXformStack);
        EXPECT_FALSE(ops.empty());
        for (auto op : ops) {
            auto attr = op.GetAttr();
            if (AL::usdmaya::xformOpToEnum(op.GetBaseName()) == AL::usdmaya::kTranslate) {
                EXPECT_EQ(0u, attr.GetNumTimeSamples());
            } else {
                EXPECT_EQ(50u, attr.GetNumTimeSamples());
            }
        }
    }

    {
        UsdPrim      prim = stage->GetPrimAtPath(SdfPath("/joint1/joint2/joint3"));
        UsdGeomXform joint3(prim);

        bool                        resetsXformStack;
        std::vector<UsdGeomXformOp> ops = joint3.GetOrderedXformOps(&resetsXformStack);
        EXPECT_EQ(ops.size(), 1u);
        for (auto op : ops) {
            auto attr = op.GetAttr();
            EXPECT_EQ(0u, attr.GetNumTimeSamples());
        }
    }
}
