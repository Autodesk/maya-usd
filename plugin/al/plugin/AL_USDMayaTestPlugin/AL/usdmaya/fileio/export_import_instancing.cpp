#include "AL/usdmaya/Metadata.h"
#include "test_usdmaya.h"

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>

using AL::maya::test::buildTempPath;

static const char* const generateInstances = R"(
{
polySphere -r 1 -sx 20 -sy 20 -ax 0 1 0 -cuv 2 -ch 1;
instance;
instance;
setAttr "pSphere2.translateZ" 5;
setAttr "pSphere3.translateX" 5;
CreateNURBSCircle;
instance;
setAttr "nurbsCircle1.translateX" 5;
createNode "transform" -n "parentTransform";
parent nurbsCircle2 parentTransform;
setAttr "nurbsCircle2.translateZ" 5;
}
)";

TEST(export_import_instancing, usd_instancing_roundtrip)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand(generateInstances);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_instances.usda");

    MString command = "file -force -options "
                      "\"Dynamic_Attributes=1;"
                      "Meshes=1;"
                      "Nurbs_Curves=1;"
                      "Duplicate_Instances=0;"
                      "Merge_Transforms=0;"
                      "Animation=1;"
                      "Use_Timeline_Range=0;"
                      "Frame_Min=1;"
                      "Frame_Max=50;"
                      "Filter_Sample=0;\" -typ \"AL usdmaya export\" -pr -ea \"";
    command += temp_path.c_str();
    command += "\";";
    MGlobal::executeCommand(command);

    UsdStageRefPtr stage = UsdStage::Open(temp_path);
    EXPECT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pSphere1"));
    EXPECT_TRUE(prim.IsValid() && prim.IsInstance() && prim.IsA<UsdGeomXform>());

    prim = stage->GetPrimAtPath(SdfPath("/pSphere2"));
    EXPECT_TRUE(prim.IsValid() && prim.IsInstance() && prim.IsA<UsdGeomXform>());
    UsdGeomXform usdXform(prim);
    GfMatrix4d   usdTransform;
    bool         resetsXformStack = false;
    usdXform.GetLocalTransformation(&usdTransform, &resetsXformStack);
    EXPECT_FLOAT_EQ(usdTransform[3][2], 5.0f);

    prim = stage->GetPrimAtPath(SdfPath("/pSphere3"));
    EXPECT_TRUE(prim.IsValid() && prim.IsInstance() && prim.IsA<UsdGeomXform>());
    usdXform = UsdGeomXform(prim);
    usdXform.GetLocalTransformation(&usdTransform, &resetsXformStack);
    EXPECT_DOUBLE_EQ(usdTransform[3][0], 5.0);

    UsdPrim masterPrim = prim.GetMaster();
    EXPECT_TRUE(masterPrim.IsValid());
    UsdPrim masterPrimChild = masterPrim.GetChild(TfToken("pSphereShape1"));
    EXPECT_TRUE(masterPrimChild.IsValid() && masterPrimChild.IsA<UsdGeomMesh>());

    prim = stage->GetPrimAtPath(SdfPath("/parentTransform/nurbsCircle2"));
    EXPECT_TRUE(prim.IsValid() && prim.IsInstance() && prim.IsA<UsdGeomXform>());
    usdXform = UsdGeomXform(prim);
    usdXform.GetLocalTransformation(&usdTransform, &resetsXformStack);
    EXPECT_DOUBLE_EQ(usdTransform[3][2], 5.0);

    MFileIO::newFile(true);
    command = "file -type \"AL usdmaya import\""
              "-i \"";
    command += temp_path.c_str();
    command += "\"";
    MGlobal::executeCommand(command);
    MSelectionList sl;
    sl.add("pSphereShape1");
    MDagPath path;
    MStatus  status = sl.getDagPath(0, path);
    EXPECT_TRUE(status == MStatus::kSuccess && path.isInstanced());
    MFnDagNode dag(path, &status);
    EXPECT_TRUE(status == MStatus::kSuccess && dag.parentCount() == 3);
    MDagPathArray allPaths;
    status = dag.getAllPaths(allPaths);
    EXPECT_TRUE(status == MStatus::kSuccess);
    ASSERT_TRUE(allPaths.length() == 3);
    EXPECT_EQ(allPaths[0].fullPathName(), "|pSphere1|pSphereShape1");
    EXPECT_EQ(allPaths[1].fullPathName(), "|pSphere2|pSphereShape1");
    EXPECT_EQ(allPaths[2].fullPathName(), "|pSphere3|pSphereShape1");

    sl.add("|parentTransform|nurbsCircle2");
    status = sl.getDagPath(1, path);
    EXPECT_EQ(status, MStatus::kSuccess);
    MFnTransform transform;
    transform.setObject(path);
    MVector translation = transform.getTranslation(MSpace::kObject);
    EXPECT_DOUBLE_EQ(translation.z, 5.0);

    sl.add("|parentTransform|nurbsCircle2|nurbsCircleShape1");
    status = sl.getDagPath(2, path);
    EXPECT_TRUE(status == MStatus::kSuccess && path.isInstanced());
    status = dag.setObject(path);
    EXPECT_TRUE(status == MStatus::kSuccess && dag.parentCount() == 2);
    status = dag.getAllPaths(allPaths);
    EXPECT_TRUE(status == MStatus::kSuccess);
    ASSERT_TRUE(allPaths.length() == 2);
    EXPECT_EQ(allPaths[0].fullPathName(), "|nurbsCircle1|nurbsCircleShape1");
    EXPECT_EQ(allPaths[1].fullPathName(), "|parentTransform|nurbsCircle2|nurbsCircleShape1");
}
