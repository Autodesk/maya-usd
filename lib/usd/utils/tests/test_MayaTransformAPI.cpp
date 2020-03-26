#include <gtest/gtest.h>
#include <fstream>
#include "../MayaTransformAPI.h"
#include <pxr/usd/usdGeom/xform.h>

const char* const fullXformDef = 
R"(#
usda 1.0
(
    defaultPrim = "maya_xform"
)

def Xform "maya_xform"
{
    float3 xformOp:rotateXYZ = (4, 5, 6)
    float3 xformOp:rotateXYZ:rotateAxis = (10, 11, 12)
    float3 xformOp:scale = (7, 8, 9)
    matrix4d xformOp:transform:shear = ( (1, 0, 0, 0), (0.1, 1, 0, 0), (0.2, 0.3, 1, 0), (0, 0, 0, 1) )
    float3 xformOp:translate:rotatePivot = (13, 14, 15)
    float3 xformOp:translate:rotatePivotINV = (-13, -14, -15)
    float3 xformOp:translate:rotatePivotTranslate = (0.19292736, 0.6936933, -0.8563779)
    float3 xformOp:translate:scalePivot = (16, 17, 18)
    float3 xformOp:translate:scalePivotINV = (-16, -17, -18)
    float3 xformOp:translate:scalePivotTranslate = (142, 167.6, 144)
    double3 xformOp:translate = (1, 2, 3)
    uniform token[] xformOpOrder = [
        "xformOp:translate", 
        "xformOp:translate:rotatePivotTranslate", 
        "xformOp:translate:rotatePivot",
        "xformOp:rotateXYZ",
        "xformOp:rotateXYZ:rotateAxis",
        "xformOp:translate:rotatePivotINV",
        "xformOp:translate:scalePivotTranslate",
        "xformOp:translate:scalePivot",
        "xformOp:transform:shear",
        "xformOp:scale", 
        "xformOp:translate:scalePivotINV"]
}
)";

TEST(MayaTransformAPI, orderCheckIsValid)
{
  #if AL_SUPPORT_LEGACY_NAMES
  // test old API orders 
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

    std::vector<UsdGeomXformOp> ops;
    auto translate = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("translate"));
    ops.push_back(translate);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto rotatePivotTranslate = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivotTranslate"));
    ops.push_back(rotatePivotTranslate);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto rotatePivot = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"));
    ops.push_back(rotatePivot);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto rotate = xform.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotate"));
    ops.push_back(rotate);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto rotateAxis = xform.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotateAxis"));
    ops.push_back(rotateAxis);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto rotatePivotINV = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"), true);
    ops.push_back(rotatePivotINV);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto scalePivotTranslate = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivotTranslate"));
    ops.push_back(scalePivotTranslate);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto scalePivot = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"));
    ops.push_back(scalePivot);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto shear = xform.AddTransformOp(UsdGeomXformOp::PrecisionDouble, TfToken("shear"));
    ops.push_back(shear);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto scale = xform.AddScaleOp(UsdGeomXformOp::PrecisionFloat, TfToken("scale"));
    ops.push_back(scale);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto scalePivotINV = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"), true);
    ops.push_back(scalePivotINV);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }
  }
  #endif

  // test new API orders 
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

    std::vector<UsdGeomXformOp> ops;
    auto translate = xform.AddTranslateOp(UsdGeomXformOp::PrecisionDouble);
    ops.push_back(translate);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto rotatePivotTranslate = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivotTranslate"));
    ops.push_back(rotatePivotTranslate);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto rotatePivot = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"));
    ops.push_back(rotatePivot);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto rotate = xform.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat);
    ops.push_back(rotate);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto rotateAxis = xform.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotateAxis"));
    ops.push_back(rotateAxis);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto rotatePivotINV = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"), true);
    ops.push_back(rotatePivotINV);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto scalePivotTranslate = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivotTranslate"));
    ops.push_back(scalePivotTranslate);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto scalePivot = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"));
    ops.push_back(scalePivot);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto shear = xform.AddTransformOp(UsdGeomXformOp::PrecisionDouble, TfToken("shear"));
    ops.push_back(shear);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto scale = xform.AddScaleOp(UsdGeomXformOp::PrecisionFloat);
    ops.push_back(scale);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }

    auto scalePivotINV = xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"), true);
    ops.push_back(scalePivotINV);
    xform.SetXformOpOrder(ops);
    {
      MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
      EXPECT_TRUE(api);
      EXPECT_EQ(MayaUsdUtils::TransformAPI::kMaya, api.api());
    }
  }
}

TEST(MayaTransformAPI, scale)
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

  MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
  EXPECT_TRUE(api);

  api.scale(GfVec3f(1, 2, 3));
  auto value = api.scale();
  EXPECT_NEAR(value[0], 1.0f, 0.1f);
  EXPECT_NEAR(value[1], 2.0f, 0.1f);
  EXPECT_NEAR(value[2], 3.0f, 0.1f);

  bool resetsXformStack;
  std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

  ASSERT_EQ(1U, ops.size());
  EXPECT_EQ(UsdGeomXformOp::TypeScale, ops[0].GetOpType());
  EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
}

TEST(MayaTransformAPI, rotateAxis)
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

  MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
  EXPECT_TRUE(api);

  api.rotateAxis(GfVec3f(1, 2, 3));
  auto value = api.rotateAxis();
  EXPECT_NEAR(value[0], 1.0f, 0.1f);
  EXPECT_NEAR(value[1], 2.0f, 0.1f);
  EXPECT_NEAR(value[2], 3.0f, 0.1f);

  bool resetsXformStack;
  std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

  ASSERT_EQ(1U, ops.size());
  EXPECT_EQ(UsdGeomXformOp::TypeRotateXYZ, ops[0].GetOpType());
  EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
}

TEST(MayaTransformAPI, translate)
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

  MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
  EXPECT_TRUE(api);

  api.translate(GfVec3f(1, 2, 3));
  auto value = api.translate();
  EXPECT_NEAR(value[0], 1.0f, 0.1f);
  EXPECT_NEAR(value[1], 2.0f, 0.1f);
  EXPECT_NEAR(value[2], 3.0f, 0.1f);

  bool resetsXformStack;
  std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

  ASSERT_EQ(1U, ops.size());
  EXPECT_EQ(UsdGeomXformOp::TypeTranslate, ops[0].GetOpType());
  EXPECT_EQ(UsdGeomXformOp::PrecisionDouble, ops[0].GetPrecision());
}

TEST(MayaTransformAPI, scalePivot)
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

  MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
  EXPECT_TRUE(api);

  api.scalePivot(GfVec3f(1, 2, 3));
  auto value = api.scalePivot();
  EXPECT_NEAR(value[0], 1.0f, 0.1f);
  EXPECT_NEAR(value[1], 2.0f, 0.1f);
  EXPECT_NEAR(value[2], 3.0f, 0.1f);

  bool resetsXformStack;
  std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

  ASSERT_EQ(2U, ops.size());
  EXPECT_EQ(UsdGeomXformOp::TypeTranslate, ops[0].GetOpType());
  EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
  EXPECT_EQ(UsdGeomXformOp::TypeTranslate, ops[1].GetOpType());
  EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[1].GetPrecision());
}

TEST(MayaTransformAPI, rotatePivot)
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

  MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
  EXPECT_TRUE(api);

  api.rotatePivot(GfVec3f(1, 2, 3));
  auto value = api.rotatePivot();
  EXPECT_NEAR(value[0], 1.0f, 0.1f);
  EXPECT_NEAR(value[1], 2.0f, 0.1f);
  EXPECT_NEAR(value[2], 3.0f, 0.1f);

  bool resetsXformStack;
  std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

  ASSERT_EQ(2U, ops.size());
  EXPECT_EQ(UsdGeomXformOp::TypeTranslate, ops[0].GetOpType());
  EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
  EXPECT_EQ(UsdGeomXformOp::TypeTranslate, ops[1].GetOpType());
  EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[1].GetPrecision());
}

TEST(MayaTransformAPI, rotatePivotTranslate)
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

  MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
  EXPECT_TRUE(api);

  api.rotatePivotTranslate(GfVec3f(1, 2, 3));
  auto value = api.rotatePivotTranslate();
  EXPECT_NEAR(value[0], 1.0f, 0.1f);
  EXPECT_NEAR(value[1], 2.0f, 0.1f);
  EXPECT_NEAR(value[2], 3.0f, 0.1f);

  bool resetsXformStack;
  std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

  ASSERT_EQ(1U, ops.size());
  EXPECT_EQ(UsdGeomXformOp::TypeTranslate, ops[0].GetOpType());
  EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
}

TEST(MayaTransformAPI, scalePivotTranslate)
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

  MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
  EXPECT_TRUE(api);

  api.scalePivotTranslate(GfVec3f(1, 2, 3));
  auto value = api.scalePivotTranslate();
  EXPECT_NEAR(value[0], 1.0f, 0.1f);
  EXPECT_NEAR(value[1], 2.0f, 0.1f);
  EXPECT_NEAR(value[2], 3.0f, 0.1f);

  bool resetsXformStack;
  std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

  ASSERT_EQ(1U, ops.size());
  EXPECT_EQ(UsdGeomXformOp::TypeTranslate, ops[0].GetOpType());
  EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
}

TEST(MayaTransformAPI, rotate)
{
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

    MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
    EXPECT_TRUE(api);

    api.rotate(GfVec3f(1, 2, 3), MayaUsdUtils::RotationOrder::kXYZ);
    auto value = api.rotate();
    EXPECT_NEAR(value[0], 1.0f, 0.1f);
    EXPECT_NEAR(value[1], 2.0f, 0.1f);
    EXPECT_NEAR(value[2], 3.0f, 0.1f);
    EXPECT_EQ(MayaUsdUtils::RotationOrder::kXYZ, api.rotateOrder());

    bool resetsXformStack;
    std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

    ASSERT_EQ(1U, ops.size());
    EXPECT_EQ(UsdGeomXformOp::TypeRotateXYZ, ops[0].GetOpType());
    EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
  }

  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

    MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
    EXPECT_TRUE(api);

    api.rotate(GfVec3f(1, 2, 3), MayaUsdUtils::RotationOrder::kXZY);
    auto value = api.rotate();
    EXPECT_NEAR(value[0], 1.0f, 0.1f);
    EXPECT_NEAR(value[1], 2.0f, 0.1f);
    EXPECT_NEAR(value[2], 3.0f, 0.1f);
    EXPECT_EQ(MayaUsdUtils::RotationOrder::kXZY, api.rotateOrder());

    bool resetsXformStack;
    std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

    ASSERT_EQ(1U, ops.size());
    EXPECT_EQ(UsdGeomXformOp::TypeRotateXZY, ops[0].GetOpType());
    EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
  }

  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

    MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
    EXPECT_TRUE(api);

    api.rotate(GfVec3f(1, 2, 3), MayaUsdUtils::RotationOrder::kYXZ);
    auto value = api.rotate();
    EXPECT_NEAR(value[0], 1.0f, 0.1f);
    EXPECT_NEAR(value[1], 2.0f, 0.1f);
    EXPECT_NEAR(value[2], 3.0f, 0.1f);
    EXPECT_EQ(MayaUsdUtils::RotationOrder::kYXZ, api.rotateOrder());

    bool resetsXformStack;
    std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

    ASSERT_EQ(1U, ops.size());
    EXPECT_EQ(UsdGeomXformOp::TypeRotateYXZ, ops[0].GetOpType());
    EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
  }

  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

    MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
    EXPECT_TRUE(api);

    api.rotate(GfVec3f(1, 2, 3), MayaUsdUtils::RotationOrder::kYZX);
    auto value = api.rotate();
    EXPECT_NEAR(value[0], 1.0f, 0.1f);
    EXPECT_NEAR(value[1], 2.0f, 0.1f);
    EXPECT_NEAR(value[2], 3.0f, 0.1f);
    EXPECT_EQ(MayaUsdUtils::RotationOrder::kYZX, api.rotateOrder());

    bool resetsXformStack;
    std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

    ASSERT_EQ(1U, ops.size());
    EXPECT_EQ(UsdGeomXformOp::TypeRotateYZX, ops[0].GetOpType());
    EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
  }

  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

    MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
    EXPECT_TRUE(api);

    api.rotate(GfVec3f(1, 2, 3), MayaUsdUtils::RotationOrder::kZXY);
    auto value = api.rotate();
    EXPECT_NEAR(value[0], 1.0f, 0.1f);
    EXPECT_NEAR(value[1], 2.0f, 0.1f);
    EXPECT_NEAR(value[2], 3.0f, 0.1f);
    EXPECT_EQ(MayaUsdUtils::RotationOrder::kZXY, api.rotateOrder());

    bool resetsXformStack;
    std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

    ASSERT_EQ(1U, ops.size());
    EXPECT_EQ(UsdGeomXformOp::TypeRotateZXY, ops[0].GetOpType());
    EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
  }

  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

    MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
    EXPECT_TRUE(api);

    api.rotate(GfVec3f(1, 2, 3), MayaUsdUtils::RotationOrder::kZYX);
    auto value = api.rotate();
    EXPECT_NEAR(value[0], 1.0f, 0.1f);
    EXPECT_NEAR(value[1], 2.0f, 0.1f);
    EXPECT_NEAR(value[2], 3.0f, 0.1f);
    EXPECT_EQ(MayaUsdUtils::RotationOrder::kZYX, api.rotateOrder());

    bool resetsXformStack;
    std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

    ASSERT_EQ(1U, ops.size());
    EXPECT_EQ(UsdGeomXformOp::TypeRotateZYX, ops[0].GetOpType());
    EXPECT_EQ(UsdGeomXformOp::PrecisionFloat, ops[0].GetPrecision());
  }

  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/maya_xform"));

    MayaUsdUtils::MayaTransformAPI api(xform.GetPrim());
    EXPECT_TRUE(api);

    api.inheritsTransform(true);
    EXPECT_EQ(true, api.inheritsTransform());

    api.inheritsTransform(false);
    EXPECT_EQ(false, api.inheritsTransform());
  }
}

const char* getDataPath(const char* const str)
{
  static char buffer[512];
  sprintf(buffer, "%s/%s.usda", AL_EXTRAS_TEST_DATA, str);
  return buffer;
}


TEST(MayaTransformAPI, asMatrix)
{
  // xyz_srt_only
  {
    const char* const path = getDataPath("xyz_srt_only");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {1.935285327, -0.501800553, 0.05354350602, 0},
      {0.657955962, 2.666353016, 1.207334066, 0},
      {-0.4990711937, -1.534204422, 3.660211023, 0},
      {-1.637680148, 1.856961273, 0.2449591934, 1}
    };
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
    // access components
    auto scale = api.scale();
    auto rotate = api.rotate();
    auto order = api.rotateOrder();
    auto translate = api.translate();
    api.setFromMatrix(computed);
    auto scaleAfter = api.scale();
    auto rotateAfter = api.rotate();
    auto orderAfter = api.rotateOrder();
    auto translateAfter = api.translate();
    EXPECT_NEAR(scale[0], scaleAfter[0], 1e-5);
    EXPECT_NEAR(scale[1], scaleAfter[1], 1e-5);
    EXPECT_NEAR(scale[2], scaleAfter[2], 1e-5);
    EXPECT_NEAR(translate[0], translateAfter[0], 1e-5);
    EXPECT_NEAR(translate[1], translateAfter[1], 1e-5);
    EXPECT_NEAR(translate[2], translateAfter[2], 1e-5);
    EXPECT_NEAR(rotate[0], rotateAfter[0], 1e-5);
    EXPECT_NEAR(rotate[1], rotateAfter[1], 1e-5);
    EXPECT_NEAR(rotate[2], rotateAfter[2], 1e-5);
    EXPECT_EQ(order, orderAfter);
  }

  // xzy_srt_only
  {
    const char* const path = getDataPath("xzy_srt_only");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {1.935285327, -0.5019804767, 0.05182955793, 0},
      {0.656673889, 2.658237488, 1.22578663, 0},
      {-0.5020641537, -1.558807841, 3.649390319, 0},
      {-1.637680148, 1.856961273, 0.2449591934, 1}
    };
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
    // access components
    auto scale = api.scale();
    auto rotate = api.rotate();
    auto order = api.rotateOrder();
    auto translate = api.translate();
    api.setFromMatrix(computed);
    auto scaleAfter = api.scale();
    auto rotateAfter = api.rotate();
    auto orderAfter = api.rotateOrder();
    auto translateAfter = api.translate();
    EXPECT_NEAR(scale[0], scaleAfter[0], 1e-5);
    EXPECT_NEAR(scale[1], scaleAfter[1], 1e-5);
    EXPECT_NEAR(scale[2], scaleAfter[2], 1e-5);
    EXPECT_NEAR(translate[0], translateAfter[0], 1e-5);
    EXPECT_NEAR(translate[1], translateAfter[1], 1e-5);
    EXPECT_NEAR(translate[2], translateAfter[2], 1e-5);
    EXPECT_NEAR(rotate[0], rotateAfter[0], 1e-5);
    EXPECT_NEAR(rotate[1], rotateAfter[1], 1e-5);
    EXPECT_NEAR(rotate[2], rotateAfter[2], 1e-5);
    EXPECT_EQ(order, orderAfter);
  }

  // yxz_srt_only
  {
    const char* const path = getDataPath("yxz_srt_only");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {1.929874975, -0.5226665623, 0.04901270024, 0},
      {0.6892549759, 2.658237488, 1.207766963, 0},
      {-0.507697869, -1.531371326, 3.660211023, 0},
      {-1.637680148, 1.856961273, 0.2449591934, 1}
    };
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
    // access components
    auto scale = api.scale();
    auto rotate = api.rotate();
    auto order = api.rotateOrder();
    auto translate = api.translate();
    api.setFromMatrix(computed);
    auto scaleAfter = api.scale();
    auto rotateAfter = api.rotate();
    auto orderAfter = api.rotateOrder();
    auto translateAfter = api.translate();
    EXPECT_NEAR(scale[0], scaleAfter[0], 1e-5);
    EXPECT_NEAR(scale[1], scaleAfter[1], 1e-5);
    EXPECT_NEAR(scale[2], scaleAfter[2], 1e-5);
    EXPECT_NEAR(translate[0], translateAfter[0], 1e-5);
    EXPECT_NEAR(translate[1], translateAfter[1], 1e-5);
    EXPECT_NEAR(translate[2], translateAfter[2], 1e-5);
    EXPECT_NEAR(rotate[0], rotateAfter[0], 1e-5);
    EXPECT_NEAR(rotate[1], rotateAfter[1], 1e-5);
    EXPECT_NEAR(rotate[2], rotateAfter[2], 1e-5);
    EXPECT_EQ(order, orderAfter);
  }


  // yzx_srt_only
  {
    const char* const path = getDataPath("yzx_srt_only");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {1.935285327, -0.4808946444, -0.1530066764, 0},
      {0.752970715, 2.658237488, 1.169105881, 0},
      {-0.1036591159, -1.585175336, 3.671031727, 0},
      {-1.637680148, 1.856961273, 0.2449591934, 1}
    };
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
    // access components
    auto scale = api.scale();
    auto rotate = api.rotate();
    auto order = api.rotateOrder();
    auto translate = api.translate();
    api.setFromMatrix(computed);
    auto scaleAfter = api.scale();
    auto rotateAfter = api.rotate();
    auto orderAfter = api.rotateOrder();
    auto translateAfter = api.translate();
    EXPECT_NEAR(scale[0], scaleAfter[0], 1e-5);
    EXPECT_NEAR(scale[1], scaleAfter[1], 1e-5);
    EXPECT_NEAR(scale[2], scaleAfter[2], 1e-5);
    EXPECT_NEAR(translate[0], translateAfter[0], 1e-5);
    EXPECT_NEAR(translate[1], translateAfter[1], 1e-5);
    EXPECT_NEAR(translate[2], translateAfter[2], 1e-5);
    EXPECT_NEAR(rotate[0], rotateAfter[0], 1e-5);
    EXPECT_NEAR(rotate[1], rotateAfter[1], 1e-5);
    EXPECT_NEAR(rotate[2], rotateAfter[2], 1e-5);
    EXPECT_EQ(order, orderAfter);
  }

  // zxy_srt_only
  {
    const char* const path = getDataPath("zxy_srt_only");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {1.940695679, -0.4595033172, -0.1501898187, 0},
      {0.7214018157, 2.658237488, 1.188845187, 0},
      {-0.09802540048, -1.61035595, 3.660211023, 0},
      {-1.637680148, 1.856961273, 0.2449591934, 1}
    };
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
    // access components
    auto scale = api.scale();
    auto rotate = api.rotate();
    auto order = api.rotateOrder();
    auto translate = api.translate();
    api.setFromMatrix(computed);
    auto scaleAfter = api.scale();
    auto rotateAfter = api.rotate();
    auto orderAfter = api.rotateOrder();
    auto translateAfter = api.translate();
    EXPECT_NEAR(scale[0], scaleAfter[0], 1e-5);
    EXPECT_NEAR(scale[1], scaleAfter[1], 1e-5);
    EXPECT_NEAR(scale[2], scaleAfter[2], 1e-5);
    EXPECT_NEAR(translate[0], translateAfter[0], 1e-5);
    EXPECT_NEAR(translate[1], translateAfter[1], 1e-5);
    EXPECT_NEAR(translate[2], translateAfter[2], 1e-5);
    EXPECT_NEAR(rotate[0], rotateAfter[0], 1e-5);
    EXPECT_NEAR(rotate[1], rotateAfter[1], 1e-5);
    EXPECT_NEAR(rotate[2], rotateAfter[2], 1e-5);
    EXPECT_EQ(order, orderAfter);
  }

  // zyx_srt_only
  {
    const char* const path = getDataPath("zyx_srt_only");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {1.935285327, -0.4803693265, -0.1546480269, 0},
      {0.7527008295, 2.65012196, 1.187558445, 0},
      {-0.107087012, -1.609778754, 3.660211023, 0},
      {-1.637680148, 1.856961273, 0.2449591934, 1}
    };
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
    // access components
    auto scale = api.scale();
    auto rotate = api.rotate();
    auto order = api.rotateOrder();
    auto translate = api.translate();
    api.setFromMatrix(computed);
    auto scaleAfter = api.scale();
    auto rotateAfter = api.rotate();
    auto orderAfter = api.rotateOrder();
    auto translateAfter = api.translate();
    EXPECT_NEAR(scale[0], scaleAfter[0], 1e-5);
    EXPECT_NEAR(scale[1], scaleAfter[1], 1e-5);
    EXPECT_NEAR(scale[2], scaleAfter[2], 1e-5);
    EXPECT_NEAR(translate[0], translateAfter[0], 1e-5);
    EXPECT_NEAR(translate[1], translateAfter[1], 1e-5);
    EXPECT_NEAR(translate[2], translateAfter[2], 1e-5);
    EXPECT_NEAR(rotate[0], rotateAfter[0], 1e-5);
    EXPECT_NEAR(rotate[1], rotateAfter[1], 1e-5);
    EXPECT_NEAR(rotate[2], rotateAfter[2], 1e-5);
    EXPECT_EQ(order, orderAfter);
  }
}

TEST(MayaTransformAPI, asMatrix2)
{
  // xyz_rot_axes
  {
    const char* const path = getDataPath("xyz_rot_axes");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {0.1869686008, 0.7665979058, -0.6143048048, 0},
      {-0.456118156, 0.6215842985, 0.6368588444, 0},
      {0.8700568775, 0.1611229677, 0.4658759697, 0},
      {0, 0, 0, 1}
    };
    api.setFromMatrix(GfMatrix4d(expected));
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
  }

  // xzy_rot_axes
  {
    const char* const path = getDataPath("xzy_rot_axes");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {0.1875377363, 0.8291052843, -0.5267010775, 0},
      {-0.4301055347, 0.551390849, 0.7148268047, 0},
      {0.8830848354, 0.09248004775, 0.4600093632, 0},
      {0, 0, 0, 1}
    };
    api.setFromMatrix(GfMatrix4d(expected));
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
  }
  // yzx_rot_axes
  {
    const char* const path = getDataPath("yzx_rot_axes");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {0.1947094843, 0.7906667212, -0.5804604661, 0},
      {-0.5368748384, 0.5811773153, 0.6115540336, 0},
      {0.820885878, 0.1925592485, 0.5376498035, 0},
      {0, 0, 0, 1}
    };
    api.setFromMatrix(GfMatrix4d(expected));
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
  }

  // yxz_rot_axes
  {
    const char* const path = getDataPath("yxz_rot_axes");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {0.1426716351, 0.782667423, -0.605868393, 0},
      {-0.4928736069, 0.5870167939, 0.6422514237, 0},
      {0.8583241883, 0.2069854794, 0.4695067616, 0},
      {0, 0, 0, 1}
    };
    api.setFromMatrix(GfMatrix4d(expected));
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
  }

  // zxy_rot_axes
  {
    const char* const path = getDataPath("zxy_rot_axes");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {0.2372957406, 0.8330603844, -0.4997010381, 0},
      {-0.477732612, 0.5479667326, 0.686661497, 0},
      {0.8458500357, 0.07578163366, 0.5280102851, 0},
      {0, 0, 0, 1}
    };
    api.setFromMatrix(GfMatrix4d(expected));
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
  }
  // zyx_rot_axes
  {
    const char* const path = getDataPath("zyx_rot_axes");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {0.190785138, 0.8500296655, -0.4909690407, 0},
      {-0.5129368029, 0.5127686965, 0.6884505066, 0},
      {0.8369569089, 0.1204899652, 0.5338401455, 0},
      {0, 0, 0, 1}
    };
    api.setFromMatrix(GfMatrix4d(expected));
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
  }
}

TEST(MayaTransformAPI, eulerXYZtoMatrix)
{
  // xyz
  {
    const double expected[3][3] = {
      {0.8799231763, 0.3720255519, -0.2955202067},
      {-0.3275796727, 0.9255641594, 0.189796061},
      {0.344131896, -0.07019954024, 0.9362933636}};
    GfVec3f r(0.2f, 0.3f, 0.4f);
    GfVec3f m[3];
    MayaUsdUtils::eulerXYZtoMatrix(r, m);
    EXPECT_NEAR(expected[0][0], m[0][0], 1e-5f);
    EXPECT_NEAR(expected[0][1], m[0][1], 1e-5f);
    EXPECT_NEAR(expected[0][2], m[0][2], 1e-5f);
    EXPECT_NEAR(expected[1][0], m[1][0], 1e-5f);
    EXPECT_NEAR(expected[1][1], m[1][1], 1e-5f);
    EXPECT_NEAR(expected[1][2], m[1][2], 1e-5f);
    EXPECT_NEAR(expected[2][0], m[2][0], 1e-5f);
    EXPECT_NEAR(expected[2][1], m[2][1], 1e-5f);
    EXPECT_NEAR(expected[2][2], m[2][2], 1e-5f);
  }
}


TEST(MayaTransformAPI, asMatrix3)
{
  // rotate_pivot_with_translate
  {
    const char* const path = getDataPath("rotate_pivot_with_translate");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {0.9362933636, 0.2896294776, -0.1986693308, 0},
      {-0.2750958473, 0.9564250858, 0.09784339501, 0},
      {0.2183506631, -0.03695701352, 0.9751703272, 0},
      {2.958846342, 2.408391391, 2.077471559, 1}
    };
    api.setFromMatrix(GfMatrix4d(expected));
    GfVec3d t = api.translate();
    EXPECT_NEAR(3.0, t[0], 1e-5);
    EXPECT_NEAR(2.5, t[1], 1e-5);
    EXPECT_NEAR(2.0, t[2], 1e-5);

    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
  }

  // full_pivots
  {
    const char* const path = getDataPath("full_pivots");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {1.304954803, 0.4036698245, -0.2768945156, 0},
      {-0.3891508167, 1.35295973, 0.1384093488, 0},
      {0.3403239536, -0.05760164303, 1.519912128, 0},
      {2.719909324, 2.078908636, 1.660217062, 1}
    };
    api.setFromMatrix(GfMatrix4d(expected));
    GfVec3d t = api.translate();
    EXPECT_NEAR(3.0, t[0], 1e-5);
    EXPECT_NEAR(2.5, t[1], 1e-5);
    EXPECT_NEAR(2.0, t[2], 1e-5);

    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
  }

  // scale_pivot_with_translate
  {
    const char* const path = getDataPath("scale_pivot_with_translate");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    MayaUsdUtils::MayaTransformAPI api(prim);
    ASSERT_TRUE(api);
    const double expected[4][4] = {
      {1.293383638, 1.238086455, -0.8912354878, 0},
      {-1.544496424, 2.355975202, 1.031460927, 0},
      {2.251177674, 0.02829022519, 3.306266587, 0},
      {-0.6525130202, -1.323996503, -6.368763312, 1}
    };
    api.setFromMatrix(GfMatrix4d(expected));
    GfMatrix4d computed = api.asMatrix();
    for(int i = 0; i < 4; ++i)
      for(int j = 0; j < 4; ++j)
        EXPECT_NEAR(expected[i][j], computed[i][j], 1e-5);
  }
}

TEST(MayaTransformAPI, convertToTRS)
{
  {
    const char* const path = getDataPath("transform_matrix");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);
    std::vector<GfMatrix4d> keyFrames(20);

    // query the matrix transforms
    {
      UsdGeomXformable xform(prim);
      for(int i = 1; i <= 20; ++i)
      {
        UsdTimeCode time(i);
        bool reset;
        xform.GetLocalTransformation(keyFrames.data() + i - 1, &reset, time);
      }
    }

    // convert to TRS
    {
      MayaUsdUtils::MayaTransformAPI api(prim, true);
      UsdGeomXformable xform(prim);
      ASSERT_TRUE(prim);
      bool reset;
      std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&reset);
      ASSERT_EQ(3U, ops.size());
      EXPECT_EQ(UsdGeomXformOp::TypeTranslate, ops[0].GetOpType());
      EXPECT_EQ(UsdGeomXformOp::TypeRotateXYZ, ops[1].GetOpType());
      EXPECT_EQ(UsdGeomXformOp::TypeScale, ops[2].GetOpType());

      for(int i = 1; i <= 20; ++i)
      {
        UsdTimeCode time(i);
        bool reset;
        GfMatrix4d m;
        xform.GetLocalTransformation(&m, &reset, time);
        for(int j = 0; j < 4; ++j)
        {
          for(int k = 0; k < 4; ++k)
          {
            EXPECT_NEAR(keyFrames[i - 1][j][k], m[j][k], 1e-5);
          }
        }
      }
    }
  }
}

const char* const commonXformDef = 
R"(#
#usda 1.0

def Xform "pCube1"
{
    float3 xformOp:rotateXYZ = (1, 2, 3)
    float3 xformOp:scale = (4, 5, 6)
    double3 xformOp:translate = (7, 8, 9)
    float3 xformOp:translate:pivot = (10, 11, 12)
    uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:translate:pivot", "xformOp:rotateXYZ", "xformOp:scale", "!invert!xformOp:translate:pivot"]
}
)";

TEST(MayaTransformAPI, commonProfile)
{
  {
    const char* const path = getDataPath("common_api");
    UsdStageRefPtr stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);

    MayaUsdUtils::MayaTransformAPI api(prim, false);

    EXPECT_TRUE(api);
    EXPECT_EQ(MayaUsdUtils::TransformAPI::kCommon, api.api());

    GfVec3f pivot = api.rotatePivot();
    EXPECT_NEAR(10.0f, pivot[0], 1e-5f);
    EXPECT_NEAR(11.0f, pivot[1], 1e-5f);
    EXPECT_NEAR(12.0f, pivot[2], 1e-5f);

    pivot[0] = 20.0f;
    pivot[1] = 21.0f;
    pivot[2] = 22.0f;
    api.rotatePivot(pivot);
    pivot[0] = pivot[1] = pivot[2] = 0;
    pivot = api.rotatePivot();
    EXPECT_NEAR(20.0f, pivot[0], 1e-5f);
    EXPECT_NEAR(21.0f, pivot[1], 1e-5f);
    EXPECT_NEAR(22.0f, pivot[2], 1e-5f);
  }
}
