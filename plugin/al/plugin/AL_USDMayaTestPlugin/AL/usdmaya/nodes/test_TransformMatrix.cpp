//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"
#include "test_usdmaya.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <maya/MAnimControl.h>
#include <maya/MDagModifier.h>
#include <maya/MEulerRotation.h>
#include <maya/MFileIO.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPxTransformationMatrix.h>
#include <maya/MQuaternion.h>
#include <maya/MSelectionList.h>
#include <maya/MVector.h>

#include <iostream>

using AL::maya::test::buildTempPath;

TEST(Transform, hasAnimation)
{
    auto constructTransformChain = []() {
        GfVec3f    v3f0(2.0f, 3.0f, 4.0f);
        GfVec3f    v3f1(2.2f, 3.0f, 4.0f);
        GfVec3d    v3d0(2.0f, 3.0f, 4.0f);
        GfVec3d    v3d1(2.2f, 3.0f, 4.0f);
        GfMatrix4d m40(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        GfMatrix4d m41(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   a0 = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomXform   a1 = UsdGeomXform::Define(stage, SdfPath("/root/anim_scale"));
        UsdGeomXform   a2 = UsdGeomXform::Define(stage, SdfPath("/root/anim_shear"));
        UsdGeomXform   a3 = UsdGeomXform::Define(stage, SdfPath("/root/anim_translate"));
        UsdGeomXform   a4 = UsdGeomXform::Define(stage, SdfPath("/root/anim_rotate"));
        UsdGeomXform   a5 = UsdGeomXform::Define(stage, SdfPath("/root/anim_matrix"));
        UsdTimeCode    k0(0);
        UsdTimeCode    k1(1);
        auto           op1 = a1.AddScaleOp(UsdGeomXformOp::PrecisionFloat, TfToken("scale"));
        auto           op2 = a2.AddTransformOp(UsdGeomXformOp::PrecisionDouble, TfToken("shear"));
        auto op3 = a3.AddTranslateOp(UsdGeomXformOp::PrecisionDouble, TfToken("translate"));
        auto op4 = a4.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotate"));
        auto op5 = a5.AddTransformOp(UsdGeomXformOp::PrecisionDouble, TfToken("transform"));
        op1.Set(v3f0, k0);
        op1.Set(v3f1, k1);
        op2.Set(m40, k0);
        op2.Set(m41, k1);
        op3.Set(v3d0, k0);
        op3.Set(v3d1, k1);
        op4.Set(v3f0, k0);
        op4.Set(v3f1, k1);
        op5.Set(m40, k0);
        op5.Set(m41, k1);
        return stage;
    };

    MFileIO::newFile(true);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_transform_animations.usda");
    std::string       sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    MString shapeName;
    {
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        MDagModifier modifier1;
        MDGModifier  modifier2;

        // construct a chain of transform nodes
        MObject leafNode = proxy->makeUsdTransforms(
            stage->GetPrimAtPath(SdfPath("/root")),
            modifier1,
            AL::usdmaya::nodes::ProxyShape::kRequested,
            &modifier2);

        // make sure we get some sane looking values.
        EXPECT_FALSE(leafNode == MObject::kNullObj);
        EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
        EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());

        MItDependencyNodes it(MFn::kPluginTransformNode);
        for (; !it.isDone(); it.next()) {
            MObject           obj = it.item();
            MFnDependencyNode fn(obj);

            AL::usdmaya::nodes::Transform* ptr = (AL::usdmaya::nodes::Transform*)fn.userNode();
            AL::usdmaya::nodes::TransformationMatrix* matrix = ptr->getTransMatrix();
            MString                                   str = ptr->primPathPlug().asString();

            if (str == "/root") {
                EXPECT_FALSE(matrix->hasAnimation());
                EXPECT_FALSE(matrix->hasAnimatedScale());
                EXPECT_FALSE(matrix->hasAnimatedShear());
                EXPECT_FALSE(matrix->hasAnimatedTranslation());
                EXPECT_FALSE(matrix->hasAnimatedRotation());
                EXPECT_FALSE(matrix->hasAnimatedMatrix());
            } else if (str == "/root/anim_scale") {
                EXPECT_TRUE(matrix->hasAnimation());
                EXPECT_TRUE(matrix->hasAnimatedScale());
                EXPECT_FALSE(matrix->hasAnimatedShear());
                EXPECT_FALSE(matrix->hasAnimatedTranslation());
                EXPECT_FALSE(matrix->hasAnimatedRotation());
                EXPECT_FALSE(matrix->hasAnimatedMatrix());
            } else if (str == "/root/anim_shear") {
                EXPECT_TRUE(matrix->hasAnimation());
                EXPECT_FALSE(matrix->hasAnimatedScale());
                EXPECT_TRUE(matrix->hasAnimatedShear());
                EXPECT_FALSE(matrix->hasAnimatedTranslation());
                EXPECT_FALSE(matrix->hasAnimatedRotation());
                EXPECT_FALSE(matrix->hasAnimatedMatrix());
            } else if (str == "/root/anim_translate") {
                EXPECT_TRUE(matrix->hasAnimation());
                EXPECT_FALSE(matrix->hasAnimatedScale());
                EXPECT_FALSE(matrix->hasAnimatedShear());
                EXPECT_TRUE(matrix->hasAnimatedTranslation());
                EXPECT_FALSE(matrix->hasAnimatedRotation());
                EXPECT_FALSE(matrix->hasAnimatedMatrix());
            } else if (str == "/root/anim_rotate") {
                EXPECT_TRUE(matrix->hasAnimation());
                EXPECT_FALSE(matrix->hasAnimatedScale());
                EXPECT_FALSE(matrix->hasAnimatedShear());
                EXPECT_FALSE(matrix->hasAnimatedTranslation());
                EXPECT_TRUE(matrix->hasAnimatedRotation());
                EXPECT_FALSE(matrix->hasAnimatedMatrix());
            } else if (str == "/root/anim_matrix") {
                EXPECT_TRUE(matrix->hasAnimation());
                EXPECT_FALSE(matrix->hasAnimatedScale());
                EXPECT_FALSE(matrix->hasAnimatedShear());
                EXPECT_FALSE(matrix->hasAnimatedTranslation());
                EXPECT_FALSE(matrix->hasAnimatedRotation());
                EXPECT_TRUE(matrix->hasAnimatedMatrix());
            }
        }
    }
}

//  inline bool primHasScale() const
//  inline bool primHasRotation() const
//  inline bool primHasTranslation() const
//  inline bool primHasShear() const
//  inline bool primHasScalePivot() const
//  inline bool primHasScalePivotTranslate() const
//  inline bool primHasRotatePivot() const
//  inline bool primHasRotatePivotTranslate() const
//  inline bool primHasRotateAxes() const
//  inline bool primHasPivot() const
//  inline bool primHasTransform() const
TEST(Transform, primHas)
{
    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   a0 = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomXform   a1 = UsdGeomXform::Define(stage, SdfPath("/root/translate"));
        UsdGeomXform   a2 = UsdGeomXform::Define(stage, SdfPath("/root/pivot"));
        UsdGeomXform   a3 = UsdGeomXform::Define(stage, SdfPath("/root/rotatePivotTranslate"));
        UsdGeomXform   a4 = UsdGeomXform::Define(stage, SdfPath("/root/rotatePivot"));
        UsdGeomXform   a5 = UsdGeomXform::Define(stage, SdfPath("/root/rotateAxis"));
        UsdGeomXform   a6 = UsdGeomXform::Define(stage, SdfPath("/root/scalePivotTranslate"));
        UsdGeomXform   a7 = UsdGeomXform::Define(stage, SdfPath("/root/scalePivot"));
        UsdGeomXform   a8 = UsdGeomXform::Define(stage, SdfPath("/root/shear"));
        UsdGeomXform   a9 = UsdGeomXform::Define(stage, SdfPath("/root/scale"));
        UsdGeomXform   aA = UsdGeomXform::Define(stage, SdfPath("/root/transform"));

        a1.AddTranslateOp(UsdGeomXformOp::PrecisionDouble, TfToken("translate"));

        a2.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("pivot"));

        a3.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivotTranslate"));

        a4.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"));
        a4.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"), true);

        a5.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotateAxis"));

        a6.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivotTranslate"));

        a7.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"));
        a7.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"), true);

        a8.AddTransformOp(UsdGeomXformOp::PrecisionDouble, TfToken("shear"));

        a9.AddScaleOp(UsdGeomXformOp::PrecisionFloat, TfToken("scale"));

        aA.AddTransformOp(UsdGeomXformOp::PrecisionDouble, TfToken("transform"));

        return stage;
    };

    MFileIO::newFile(true);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_transform_primHas.usda");
    std::string       sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    MString shapeName;
    {
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        MDagModifier modifier1;
        MDGModifier  modifier2;

        // construct a chain of transform nodes
        MObject leafNode = proxy->makeUsdTransforms(
            stage->GetPrimAtPath(SdfPath("/root")),
            modifier1,
            AL::usdmaya::nodes::ProxyShape::kRequested,
            &modifier2);

        // make sure we get some sane looking values.
        EXPECT_FALSE(leafNode == MObject::kNullObj);
        EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
        EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());

        MItDependencyNodes it(MFn::kPluginTransformNode);
        for (; !it.isDone(); it.next()) {
            MObject           obj = it.item();
            MFnDependencyNode fn(obj);

            AL::usdmaya::nodes::Transform* ptr = (AL::usdmaya::nodes::Transform*)fn.userNode();
            AL::usdmaya::nodes::TransformationMatrix* matrix = ptr->getTransMatrix();
            MString                                   str = ptr->primPathPlug().asString();

            if (str == "/root") {
                EXPECT_FALSE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_FALSE(matrix->primHasTranslation());
                EXPECT_FALSE(matrix->primHasShear());
                EXPECT_FALSE(matrix->primHasScalePivot());
                EXPECT_FALSE(matrix->primHasScalePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotatePivot());
                EXPECT_FALSE(matrix->primHasRotatePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotateAxes());
                EXPECT_FALSE(matrix->primHasPivot());
                EXPECT_FALSE(matrix->primHasTransform());
            } else if (str == "/root/translate") {
                EXPECT_FALSE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_TRUE(matrix->primHasTranslation());
                EXPECT_FALSE(matrix->primHasShear());
                EXPECT_FALSE(matrix->primHasScalePivot());
                EXPECT_FALSE(matrix->primHasScalePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotatePivot());
                EXPECT_FALSE(matrix->primHasRotatePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotateAxes());
                EXPECT_FALSE(matrix->primHasPivot());
                EXPECT_FALSE(matrix->primHasTransform());
            } else if (str == "/root/pivot") {
                EXPECT_FALSE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_FALSE(matrix->primHasTranslation());
                EXPECT_FALSE(matrix->primHasShear());
                EXPECT_FALSE(matrix->primHasScalePivot());
                EXPECT_FALSE(matrix->primHasScalePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotatePivot());
                EXPECT_FALSE(matrix->primHasRotatePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotateAxes());
                EXPECT_TRUE(matrix->primHasPivot());
                EXPECT_FALSE(matrix->primHasTransform());
            } else if (str == "/root/rotatePivotTranslate") {
                EXPECT_FALSE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_FALSE(matrix->primHasTranslation());
                EXPECT_FALSE(matrix->primHasShear());
                EXPECT_FALSE(matrix->primHasScalePivot());
                EXPECT_FALSE(matrix->primHasScalePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotatePivot());
                EXPECT_TRUE(matrix->primHasRotatePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotateAxes());
                EXPECT_FALSE(matrix->primHasPivot());
                EXPECT_FALSE(matrix->primHasTransform());
            } else if (str == "/root/rotatePivot") {
                EXPECT_FALSE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_FALSE(matrix->primHasTranslation());
                EXPECT_FALSE(matrix->primHasShear());
                EXPECT_FALSE(matrix->primHasScalePivot());
                EXPECT_FALSE(matrix->primHasScalePivotTranslate());
                EXPECT_TRUE(matrix->primHasRotatePivot());
                EXPECT_FALSE(matrix->primHasRotatePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotateAxes());
                EXPECT_FALSE(matrix->primHasPivot());
                EXPECT_FALSE(matrix->primHasTransform());
            } else if (str == "/root/rotateAxis") {
                EXPECT_FALSE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_FALSE(matrix->primHasTranslation());
                EXPECT_FALSE(matrix->primHasShear());
                EXPECT_FALSE(matrix->primHasScalePivot());
                EXPECT_FALSE(matrix->primHasScalePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotatePivot());
                EXPECT_FALSE(matrix->primHasRotatePivotTranslate());
                EXPECT_TRUE(matrix->primHasRotateAxes());
                EXPECT_FALSE(matrix->primHasPivot());
                EXPECT_FALSE(matrix->primHasTransform());
            } else if (str == "/root/scalePivotTranslate") {
                EXPECT_FALSE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_FALSE(matrix->primHasTranslation());
                EXPECT_FALSE(matrix->primHasShear());
                EXPECT_FALSE(matrix->primHasScalePivot());
                EXPECT_TRUE(matrix->primHasScalePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotatePivot());
                EXPECT_FALSE(matrix->primHasRotatePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotateAxes());
                EXPECT_FALSE(matrix->primHasPivot());
                EXPECT_FALSE(matrix->primHasTransform());
            } else if (str == "/root/scalePivot") {
                EXPECT_FALSE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_FALSE(matrix->primHasTranslation());
                EXPECT_FALSE(matrix->primHasShear());
                EXPECT_TRUE(matrix->primHasScalePivot());
                EXPECT_FALSE(matrix->primHasScalePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotatePivot());
                EXPECT_FALSE(matrix->primHasRotatePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotateAxes());
                EXPECT_FALSE(matrix->primHasPivot());
                EXPECT_FALSE(matrix->primHasTransform());
            } else if (str == "/root/shear") {
                EXPECT_FALSE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_FALSE(matrix->primHasTranslation());
                EXPECT_TRUE(matrix->primHasShear());
                EXPECT_FALSE(matrix->primHasScalePivot());
                EXPECT_FALSE(matrix->primHasScalePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotatePivot());
                EXPECT_FALSE(matrix->primHasRotatePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotateAxes());
                EXPECT_FALSE(matrix->primHasPivot());
                EXPECT_FALSE(matrix->primHasTransform());
            } else if (str == "/root/scale") {
                EXPECT_TRUE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_FALSE(matrix->primHasTranslation());
                EXPECT_FALSE(matrix->primHasShear());
                EXPECT_FALSE(matrix->primHasScalePivot());
                EXPECT_FALSE(matrix->primHasScalePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotatePivot());
                EXPECT_FALSE(matrix->primHasRotatePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotateAxes());
                EXPECT_FALSE(matrix->primHasPivot());
                EXPECT_FALSE(matrix->primHasTransform());
            } else if (str == "/root/transform") {
                EXPECT_FALSE(matrix->primHasScale());
                EXPECT_FALSE(matrix->primHasRotation());
                EXPECT_FALSE(matrix->primHasTranslation());
                EXPECT_FALSE(matrix->primHasShear());
                EXPECT_FALSE(matrix->primHasScalePivot());
                EXPECT_FALSE(matrix->primHasScalePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotatePivot());
                EXPECT_FALSE(matrix->primHasRotatePivotTranslate());
                EXPECT_FALSE(matrix->primHasRotateAxes());
                EXPECT_FALSE(matrix->primHasPivot());
                EXPECT_TRUE(matrix->primHasTransform());
            }
        }
    }
}

//  static bool pushVector(const MVector& input, UsdGeomXformOp& op, UsdTimeCode timeCode =
//  UsdTimeCode::Default()); static bool pushPoint(const MPoint& input, UsdGeomXformOp& op,
//  UsdTimeCode timeCode = UsdTimeCode::Default()); static bool pushRotation(const MEulerRotation&
//  input, UsdGeomXformOp& op, UsdTimeCode timeCode = UsdTimeCode::Default()); static void
//  pushDouble(const double input, UsdGeomXformOp& op, UsdTimeCode timeCode =
//  UsdTimeCode::Default()); static bool pushShear(const MVector& input, UsdGeomXformOp& op,
//  UsdTimeCode timeCode = UsdTimeCode::Default()); static bool pushMatrix(const MMatrix& input,
//  UsdGeomXformOp& op, UsdTimeCode timeCode = UsdTimeCode::Default()); void pushToPrim();
TEST(Transform, primValuesPushedToUsdMatchMaya)
{
    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   a = UsdGeomXform::Define(stage, SdfPath("/tm"));

        std::vector<UsdGeomXformOp> ops;
        ops.push_back(a.AddTranslateOp(UsdGeomXformOp::PrecisionDouble, TfToken("translate")));
        ops.push_back(
            a.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivotTranslate")));
        ops.push_back(a.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot")));
        ops.push_back(a.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotate")));
        ops.push_back(a.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotateAxis")));
        ops.push_back(
            a.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"), true));
        ops.push_back(
            a.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivotTranslate")));
        ops.push_back(a.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot")));
        ops.push_back(a.AddTransformOp(UsdGeomXformOp::PrecisionDouble, TfToken("shear")));
        ops.push_back(a.AddScaleOp(UsdGeomXformOp::PrecisionFloat, TfToken("scale")));
        ops.push_back(
            a.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"), true));
        a.SetXformOpOrder(ops);

        return stage;
    };

    MFileIO::newFile(true);

    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_transform_primValuesPushedToUsdMatchMaya.usda");
    std::string sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    MString shapeName;
    {
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        MDagModifier modifier1;
        MDGModifier  modifier2;

        // construct a chain of transform nodes
        MObject leafNode = proxy->makeUsdTransforms(
            stage->GetPrimAtPath(SdfPath("/tm")),
            modifier1,
            AL::usdmaya::nodes::ProxyShape::kRequested,
            &modifier2);

        // make sure we get some sane looking values.
        EXPECT_FALSE(leafNode == MObject::kNullObj);
        EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
        EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());

        MFnTransform fnx(leafNode);

        AL::usdmaya::nodes::Transform* transformNode
            = (AL::usdmaya::nodes::Transform*)fnx.userNode();

        transformNode->pushToPrimPlug().setValue(true);
        transformNode->readAnimatedValuesPlug().setValue(false);

        UsdGeomXform usd_xform(stage->GetPrimAtPath(SdfPath("/tm")));

        bool                        reset;
        std::vector<UsdGeomXformOp> ops = usd_xform.GetOrderedXformOps(&reset);
        EXPECT_EQ(11u, ops.size());

        UsdGeomXformOp& translate = ops[0];
        UsdGeomXformOp& rotatePivotTranslate = ops[1];
        UsdGeomXformOp& rotatePivot = ops[2];
        UsdGeomXformOp& rotate = ops[3];
        UsdGeomXformOp& rotateAxis = ops[4];
        UsdGeomXformOp& rotatePivotINV = ops[5];
        UsdGeomXformOp& scalePivotTranslate = ops[6];
        UsdGeomXformOp& scalePivot = ops[7];
        UsdGeomXformOp& scale = ops[9];
        UsdGeomXformOp& scalePivotINV = ops[10];

        auto randf = [](float mn, float mx) { return mn + (mx - mn) * (float(rand()) / RAND_MAX); };

        MPlug wsmPlug = fnx.findPlug("m");

        // Throw some random values at the Maya transform, and ensure those values are correctly
        // passed into USD
        for (int i = 0; i < 100; ++i) {
            // translate
            {
                MVector v(randf(-20.0f, 20.0f), randf(-20.0f, 20.0f), randf(-20.0f, 20.0f));
                fnx.setTranslation(v, MSpace::kTransform);

                GfVec3d t(0, 0, 0);
                translate.Get(&t);

                stage->Export(temp_path, false);
                EXPECT_NEAR(t[0], v.x, 1e-5f);
                EXPECT_NEAR(t[1], v.y, 1e-5f);
                EXPECT_NEAR(t[2], v.z, 1e-5f);

                // Make tiny changes within tolerance (1e-07)
                fnx.setTranslation({ v.x + 1e-8, v.y + 1e-8, v.z + 1e-8 }, MSpace::kTransform);
                // The new values will not be update on the prim
                GfVec3d m(0, 0, 0);
                translate.Get(&m);
                // Expect the values still be the same as previous by checking a even smaller
                // tolerance (1e-9)
                EXPECT_NEAR(t[0], m[0], 1e-9f);
                EXPECT_NEAR(t[1], m[1], 1e-9f);
                EXPECT_NEAR(t[2], m[2], 1e-9f);
            }

            // rotatePivotTranslate
            {
                MVector v(randf(-20.0f, 20.0f), randf(-20.0f, 20.0f), randf(-20.0f, 20.0f));

                fnx.setRotatePivotTranslation(v, MSpace::kTransform);

                GfVec3f t(0, 0, 0);
                rotatePivotTranslate.Get(&t);

                EXPECT_NEAR(t[0], v.x, 1e-5f);
                EXPECT_NEAR(t[1], v.y, 1e-5f);
                EXPECT_NEAR(t[2], v.z, 1e-5f);

                // Make tiny changes within tolerance (1e-07)
                fnx.setRotatePivotTranslation(
                    { v.x + 1e-8, v.y + 1e-8, v.z + 1e-8 }, MSpace::kTransform);
                // The new values will not be update on the prim
                GfVec3f m(0, 0, 0);
                rotatePivotTranslate.Get(&m);
                // Expect the values still be the same as previous by checking a even smaller
                // tolerance (1e-9)
                EXPECT_NEAR(t[0], m[0], 1e-9f);
                EXPECT_NEAR(t[1], m[1], 1e-9f);
                EXPECT_NEAR(t[2], m[2], 1e-9f);
            }

            // rotatePivot
            {
                MPoint v(randf(-20.0f, 20.0f), randf(-20.0f, 20.0f), randf(-20.0f, 20.0f));

                fnx.setRotatePivot(v, MSpace::kTransform, false);

                GfVec3f t(0, 0, 0);
                rotatePivot.Get(&t);
                EXPECT_NEAR(t[0], v.x, 1e-5f);
                EXPECT_NEAR(t[1], v.y, 1e-5f);
                EXPECT_NEAR(t[2], v.z, 1e-5f);

                // Make tiny changes within tolerance (1e-07)
                fnx.setRotatePivot(
                    { v.x + 1e-8, v.y + 1e-8, v.z + 1e-8 }, MSpace::kTransform, false);
                // The new values will not be update on the prim
                GfVec3f m(0, 0, 0);
                rotatePivot.Get(&m);
                // Expect the values still be the same as previous by checking a even smaller
                // tolerance (1e-9)
                EXPECT_NEAR(t[0], m[0], 1e-9f);
                EXPECT_NEAR(t[1], m[1], 1e-9f);
                EXPECT_NEAR(t[2], m[2], 1e-9f);

                rotatePivotINV.Get(&t);
                EXPECT_NEAR(t[0], v.x, 1e-5f);
                EXPECT_NEAR(t[1], v.y, 1e-5f);
                EXPECT_NEAR(t[2], v.z, 1e-5f);
            }

            // rotate
            {
                MEulerRotation r(randf(-20.0f, 20.0f), randf(-20.0f, 20.0f), randf(-20.0f, 20.0f));

                fnx.setRotation(r);

                GfVec3f t(0, 0, 0);
                rotate.Get(&t);

                const float radToDeg = 180.0f / 3.141592654f;
                EXPECT_NEAR(t[0], r.x * radToDeg, 1e-2f);
                EXPECT_NEAR(t[1], r.y * radToDeg, 1e-2f);
                EXPECT_NEAR(t[2], r.z * radToDeg, 1e-2f);

                // Make tiny changes within tolerance (1e-07)
                fnx.setRotation({ r.x + 1e-8, r.y + 1e-8, r.z + 1e-8 });
                // The new values will not be update on the prim
                GfVec3f m(0, 0, 0);
                rotate.Get(&m);
                // Notice that the values here are in degree, pick 1e-3 as abs error instead
                EXPECT_NEAR(t[0], m[0], 1e-3f);
                EXPECT_NEAR(t[1], m[1], 1e-3f);
                EXPECT_NEAR(t[2], m[2], 1e-3f);
            }

            // rotateAxis
            {
                MEulerRotation r(randf(-20.0f, 20.0f), randf(-20.0f, 20.0f), randf(-20.0f, 20.0f));
                MQuaternion    q = r.asQuaternion();

                fnx.setRotateOrientation(q, MSpace::kTransform, false);

                GfVec3f t(0, 0, 0);
                rotateAxis.Get(&t);

                MQuaternion xyz;
                const float degToRad = 3.141592654f / 180.0f;
                xyz = MEulerRotation(t[0] * degToRad, t[1] * degToRad, t[2] * degToRad);

                const double dp = (q.x * xyz.x) + (q.y * xyz.y) + (q.z * xyz.z) + (q.w * xyz.w);
                if (dp < 0) {
                    q.x = -q.x;
                    q.y = -q.y;
                    q.z = -q.z;
                    q.w = -q.w;
                }

                EXPECT_NEAR(xyz.x, q.x, 1e-5f);
                EXPECT_NEAR(xyz.y, q.y, 1e-5f);
                EXPECT_NEAR(xyz.z, q.z, 1e-5f);
                EXPECT_NEAR(xyz.w, q.w, 1e-5f);
            }

            // scalePivotTranslate
            {
                MVector v(randf(-20.0f, 20.0f), randf(-20.0f, 20.0f), randf(-20.0f, 20.0f));

                fnx.setScalePivotTranslation(v, MSpace::kTransform);

                GfVec3f t(0, 0, 0);
                scalePivotTranslate.Get(&t);

                EXPECT_NEAR(t[0], v.x, 1e-5f);
                EXPECT_NEAR(t[1], v.y, 1e-5f);
                EXPECT_NEAR(t[2], v.z, 1e-5f);

                // Make tiny changes within tolerance (1e-07)
                fnx.setScalePivotTranslation(
                    { v.x + 1e-8, v.y + 1e-8, v.z + 1e-8 }, MSpace::kTransform);
                // The new values will not be update on the prim
                GfVec3f m(0, 0, 0);
                scalePivotTranslate.Get(&m);
                // Expect the values still be the same as previous by checking a even smaller
                // tolerance (1e-9)
                EXPECT_NEAR(t[0], m[0], 1e-9f);
                EXPECT_NEAR(t[1], m[1], 1e-9f);
                EXPECT_NEAR(t[2], m[2], 1e-9f);
            }

            // scalePivot
            {
                MPoint v(randf(-20.0f, 20.0f), randf(-20.0f, 20.0f), randf(-20.0f, 20.0f));

                fnx.setScalePivot(v, MSpace::kTransform, false);

                GfVec3f t(0, 0, 0);
                scalePivot.Get(&t);

                EXPECT_NEAR(t[0], v.x, 1e-5f);
                EXPECT_NEAR(t[1], v.y, 1e-5f);
                EXPECT_NEAR(t[2], v.z, 1e-5f);

                // Make tiny changes within tolerance (1e-07)
                fnx.setScalePivot(
                    { v.x + 1e-8, v.y + 1e-8, v.z + 1e-8 }, MSpace::kTransform, false);
                // The new values will not be update on the prim
                GfVec3f m(0, 0, 0);
                scalePivot.Get(&m);
                // Expect the values still be the same as previous by checking a even smaller
                // tolerance (1e-9)
                EXPECT_NEAR(t[0], m[0], 1e-9f);
                EXPECT_NEAR(t[1], m[1], 1e-9f);
                EXPECT_NEAR(t[2], m[2], 1e-9f);

                scalePivotINV.Get(&t);

                EXPECT_NEAR(t[0], v.x, 1e-5f);
                EXPECT_NEAR(t[1], v.y, 1e-5f);
                EXPECT_NEAR(t[2], v.z, 1e-5f);
            }

            // scale
            {
                double v[] = { randf(-20.0f, 20.0f), randf(-20.0f, 20.0f), randf(-20.0f, 20.0f) };

                fnx.setScale(v);

                GfVec3f t(0, 0, 0);
                scale.Get(&t);

                EXPECT_NEAR(t[0], v[0], 1e-5f);
                EXPECT_NEAR(t[1], v[1], 1e-5f);
                EXPECT_NEAR(t[2], v[2], 1e-5f);

                // Make tiny changes within tolerance (1e-07)
                v[0] += 1e-8;
                v[1] += 1e-8;
                v[2] += 1e-8;
                fnx.setScale(v);
                // The new values will not be update on the prim
                GfVec3f m(0, 0, 0);
                scale.Get(&m);
                // Expect the values still be the same as previous by checking a even smaller
                // tolerance (1e-9)
                EXPECT_NEAR(t[0], m[0], 1e-9f);
                EXPECT_NEAR(t[1], m[1], 1e-9f);
                EXPECT_NEAR(t[2], m[2], 1e-9f);
            }

            // Just sanity check that the matrices in maya and usd evaluate the same result
            MMatrix wsm;
            MObject oMatrix;
            wsmPlug.getValue(oMatrix);
            MPlug         wsmPlug = fnx.findPlug("m");
            MFnMatrixData fnMatrix(oMatrix);
            wsm = fnMatrix.matrix();

            GfMatrix4d transform;
            bool       resetsXformStack = false;
            usd_xform.GetLocalTransformation(&transform, &resetsXformStack);

            EXPECT_NEAR(transform[0][0], wsm[0][0], 1e-3f);
            EXPECT_NEAR(transform[0][1], wsm[0][1], 1e-3f);
            EXPECT_NEAR(transform[0][2], wsm[0][2], 1e-3f);
            EXPECT_NEAR(transform[0][3], wsm[0][3], 1e-3f);

            EXPECT_NEAR(transform[1][0], wsm[1][0], 1e-3f);
            EXPECT_NEAR(transform[1][1], wsm[1][1], 1e-3f);
            EXPECT_NEAR(transform[1][2], wsm[1][2], 1e-3f);
            EXPECT_NEAR(transform[1][3], wsm[1][3], 1e-3f);

            EXPECT_NEAR(transform[2][0], wsm[2][0], 1e-3f);
            EXPECT_NEAR(transform[2][1], wsm[2][1], 1e-3f);
            EXPECT_NEAR(transform[2][2], wsm[2][2], 1e-3f);
            EXPECT_NEAR(transform[2][3], wsm[2][3], 1e-3f);

            EXPECT_NEAR(transform[3][0], wsm[3][0], 1e-3f);
            EXPECT_NEAR(transform[3][1], wsm[3][1], 1e-3f);
            EXPECT_NEAR(transform[3][2], wsm[3][2], 1e-3f);
            EXPECT_NEAR(transform[3][3], wsm[3][3], 1e-3f);
        }
    }
}

//  static bool readVector(MVector& result, const UsdGeomXformOp& op, UsdTimeCode timeCode =
//  UsdTimeCode::Default()); static bool readShear(MVector& result, const UsdGeomXformOp& op,
//  UsdTimeCode timeCode = UsdTimeCode::Default()); static bool readPoint(MPoint& result, const
//  UsdGeomXformOp& op, UsdTimeCode timeCode = UsdTimeCode::Default()); static bool
//  readRotation(MEulerRotation& result, const UsdGeomXformOp& op, UsdTimeCode timeCode =
//  UsdTimeCode::Default()); static double readDouble(const UsdGeomXformOp& op, UsdTimeCode timeCode
//  = UsdTimeCode::Default()); static bool readMatrix(MMatrix& result, const UsdGeomXformOp& op,
//  UsdTimeCode timeCode = UsdTimeCode::Default()); void updateToTime(const UsdTimeCode& time);
TEST(Transform, animationValuesFromUsdAreCorrectlyRead)
{
    std::vector<GfVec3d> translateValues;
    std::vector<GfVec3f> scaleValues;
    std::vector<GfVec3f> rotateValues;
    auto constructTransformChain = [&translateValues, &scaleValues, &rotateValues]() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   a = UsdGeomXform::Define(stage, SdfPath("/tm"));

        std::vector<UsdGeomXformOp> ops;
        ops.push_back(a.AddTranslateOp(UsdGeomXformOp::PrecisionDouble, TfToken("translate")));
        ops.push_back(a.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotate")));
        ops.push_back(a.AddScaleOp(UsdGeomXformOp::PrecisionFloat, TfToken("scale")));
        a.SetXformOpOrder(ops);

        UsdGeomXformOp& translate = ops[0];
        UsdGeomXformOp& rotate = ops[1];
        UsdGeomXformOp& scale = ops[2];

        auto randf = [](float mn, float mx) { return mn + (mx - mn) * (float(rand()) / RAND_MAX); };

        // set some random animated values in the usd file
        for (int i = 0; i < 50; ++i) {
            UsdTimeCode time(i);
            GfVec3d     t(randf(-20.0f, 20.0f), randf(-20.0f, 20.0f), randf(-20.0f, 20.0f));
            GfVec3f     r(randf(-20.0f, 20.0f), randf(-20.0f, 20.0f), randf(-20.0f, 20.0f));
            GfVec3f     s(randf(.1f, 20.0f), randf(.1f, 20.0f), randf(.1f, 20.0f));

            translateValues.push_back(t);
            scaleValues.push_back(s);
            rotateValues.push_back(r);

            translate.Set(t, time);
            scale.Set(s, time);
            rotate.Set(r, time);
        }

        return stage;
    };

    MFileIO::newFile(true);

    // In 'off' (DG) mode, setCurrentTime does not seem to trigger an eval.
    // Force it to 'parallel' for now.
    MGlobal::executeCommand(MString("evaluationManager -mode \"parallel\";"));

    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_transform_animationValuesFromUsdAreCorrectlyRead.usda");
    std::string sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    MString shapeName;
    {
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        {
            MGlobal::executeCommand(
                MString("connectAttr -f \"time1.outTime\" \"") + fn.name() + ".time\";");
        }

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        MDagModifier modifier1;
        MDGModifier  modifier2;

        // construct a chain of transform nodes
        MObject leafNode = proxy->makeUsdTransforms(
            stage->GetPrimAtPath(SdfPath("/tm")),
            modifier1,
            AL::usdmaya::nodes::ProxyShape::kRequested,
            &modifier2);

        // make sure we get some sane looking values.
        EXPECT_FALSE(leafNode == MObject::kNullObj);
        EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
        EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());

        MFnTransform fnx(leafNode);

        AL::usdmaya::nodes::Transform* transformNode
            = (AL::usdmaya::nodes::Transform*)fnx.userNode();
        AL::usdmaya::nodes::TransformationMatrix* transformMatrix = transformNode->getTransMatrix();

        transformNode->pushToPrimPlug().setValue(false);
        transformNode->readAnimatedValuesPlug().setValue(true);

        UsdGeomXform usd_xform(stage->GetPrimAtPath(SdfPath("/tm")));

        bool                        reset;
        std::vector<UsdGeomXformOp> ops = usd_xform.GetOrderedXformOps(&reset);
        EXPECT_EQ(3u, ops.size());

        MPlug wsmPlug = fnx.findPlug("m");

        // if we don't re-enable the refresh for this test, the scene won't get updated when calling
        // view frame
        if (MGlobal::kInteractive == MGlobal::mayaState())
            MGlobal::executeCommand("refresh -suspend false");

        {
            MTime time(-1, MTime::uiUnit());
            MAnimControl::setCurrentTime(time);
        }

        // set some random animated values in the usd file
        for (int i = 0; i < 50; ++i) {
            MTime time(i, MTime::uiUnit());
            MAnimControl::setCurrentTime(time);

            MObject oMatrix;
            wsmPlug.getValue(oMatrix);
            MFnMatrixData fnMatrix(oMatrix);
            fnMatrix.matrix();

            EXPECT_NEAR(transformMatrix->getTimeCode().GetValue(), i, 1e-5f);

            MVector T = fnx.getTranslation(MSpace::kTransform);
            EXPECT_NEAR(translateValues[i][0], T.x, 1e-5f);
            EXPECT_NEAR(translateValues[i][1], T.y, 1e-5f);
            EXPECT_NEAR(translateValues[i][2], T.z, 1e-5f);

            const float    degToRad = 3.141592654f / 180.0f;
            MEulerRotation rotation;
            fnx.getRotation(rotation);
            EXPECT_NEAR(degToRad * rotateValues[i][0], rotation.x, 1e-5f);
            EXPECT_NEAR(degToRad * rotateValues[i][1], rotation.y, 1e-5f);
            EXPECT_NEAR(degToRad * rotateValues[i][2], rotation.z, 1e-5f);

            double s[3];
            fnx.getScale(s);
            EXPECT_NEAR(scaleValues[i][0], s[0], 1e-5f);
            EXPECT_NEAR(scaleValues[i][1], s[1], 1e-5f);
            EXPECT_NEAR(scaleValues[i][2], s[2], 1e-5f);
        }

        {
            auto timePlug = transformNode->timePlug();
            auto timeOffsetPlug = transformNode->timeOffsetPlug();
            auto timeScalarPlug = transformNode->timeScalarPlug();
            auto outTimePlug = transformNode->outTimePlug();

            // no retest with a time offset of 2
            MTime timeOffset(2.0, MTime::uiUnit());
            timeOffsetPlug.setValue(timeOffset);
            for (int i = 2; i < 50; ++i) {
                MTime time(i, MTime::uiUnit());
                MGlobal::viewFrame(time);

                MObject oMatrix;
                wsmPlug.getValue(oMatrix);
                MFnMatrixData fnMatrix(oMatrix);
                fnMatrix.matrix();

                EXPECT_NEAR(transformMatrix->getTimeCode().GetValue(), i - 2, 1e-5f);

                MTime offsetTime = outTimePlug.asMTime();
                EXPECT_NEAR(offsetTime.value(), time.value() - 2.0, 1e-5f);

                MVector T = fnx.getTranslation(MSpace::kTransform);
                EXPECT_NEAR(translateValues[i - 2][0], T.x, 1e-5f);
                EXPECT_NEAR(translateValues[i - 2][1], T.y, 1e-5f);
                EXPECT_NEAR(translateValues[i - 2][2], T.z, 1e-5f);

                const float    degToRad = 3.141592654f / 180.0f;
                MEulerRotation rotation;
                fnx.getRotation(rotation);
                EXPECT_NEAR(degToRad * rotateValues[i - 2][0], rotation.x, 1e-5f);
                EXPECT_NEAR(degToRad * rotateValues[i - 2][1], rotation.y, 1e-5f);
                EXPECT_NEAR(degToRad * rotateValues[i - 2][2], rotation.z, 1e-5f);

                double s[3];
                fnx.getScale(s);
                EXPECT_NEAR(scaleValues[i - 2][0], s[0], 1e-5f);
                EXPECT_NEAR(scaleValues[i - 2][1], s[1], 1e-5f);
                EXPECT_NEAR(scaleValues[i - 2][2], s[2], 1e-5f);
            }

            // no retest with a time scalar of 2
            timeScalarPlug.setValue(2.0);
            MTime zeroTime(0.0, MTime::uiUnit());
            timeOffsetPlug.setValue(zeroTime);
            for (int i = 0; i < 25; ++i) {
                MTime time(i, MTime::uiUnit());
                MGlobal::viewFrame(time);

                MObject oMatrix;
                wsmPlug.getValue(oMatrix);
                MFnMatrixData fnMatrix(oMatrix);
                fnMatrix.matrix();

                EXPECT_NEAR(transformMatrix->getTimeCode().GetValue(), i * 2.0, 1e-5f);

                MTime offsetTime = outTimePlug.asMTime();
                EXPECT_NEAR(offsetTime.value(), time.value() * 2.0, 1e-5f);

                MVector T = fnx.getTranslation(MSpace::kTransform);
                EXPECT_NEAR(translateValues[i * 2][0], T.x, 1e-5f);
                EXPECT_NEAR(translateValues[i * 2][1], T.y, 1e-5f);
                EXPECT_NEAR(translateValues[i * 2][2], T.z, 1e-5f);

                const float    degToRad = 3.141592654f / 180.0f;
                MEulerRotation rotation;
                fnx.getRotation(rotation);
                EXPECT_NEAR(degToRad * rotateValues[i * 2][0], rotation.x, 1e-5f);
                EXPECT_NEAR(degToRad * rotateValues[i * 2][1], rotation.y, 1e-5f);
                EXPECT_NEAR(degToRad * rotateValues[i * 2][2], rotation.z, 1e-5f);

                double s[3];
                fnx.getScale(s);
                EXPECT_NEAR(scaleValues[i * 2][0], s[0], 1e-5f);
                EXPECT_NEAR(scaleValues[i * 2][1], s[1], 1e-5f);
                EXPECT_NEAR(scaleValues[i * 2][2], s[2], 1e-5f);
            }
            timeScalarPlug.setValue(1.0);
        }

        // now perform the same tests, but this time by modifying the time params on the proxy shape
        {
            auto timeOffsetPlug = proxy->timeOffsetPlug();
            auto timeScalarPlug = proxy->timeScalarPlug();
            auto outTimePlug = transformNode->outTimePlug();

            // no retest with a time offset of 2
            MTime timeOffset(2.0, MTime::uiUnit());
            timeOffsetPlug.setValue(timeOffset);
            for (int i = 2; i < 50; ++i) {
                MTime time(i, MTime::uiUnit());
                MGlobal::viewFrame(time);

                MObject oMatrix;
                wsmPlug.getValue(oMatrix);
                MFnMatrixData fnMatrix(oMatrix);
                fnMatrix.matrix();

                EXPECT_NEAR(transformMatrix->getTimeCode().GetValue(), i - 2, 1e-5f);

                MTime offsetTime = outTimePlug.asMTime();
                EXPECT_NEAR(offsetTime.value(), time.value() - 2.0, 1e-5f);

                MVector T = fnx.getTranslation(MSpace::kTransform);
                EXPECT_NEAR(translateValues[i - 2][0], T.x, 1e-5f);
                EXPECT_NEAR(translateValues[i - 2][1], T.y, 1e-5f);
                EXPECT_NEAR(translateValues[i - 2][2], T.z, 1e-5f);

                const float    degToRad = 3.141592654f / 180.0f;
                MEulerRotation rotation;
                fnx.getRotation(rotation);
                EXPECT_NEAR(degToRad * rotateValues[i - 2][0], rotation.x, 1e-5f);
                EXPECT_NEAR(degToRad * rotateValues[i - 2][1], rotation.y, 1e-5f);
                EXPECT_NEAR(degToRad * rotateValues[i - 2][2], rotation.z, 1e-5f);

                double s[3];
                fnx.getScale(s);
                EXPECT_NEAR(scaleValues[i - 2][0], s[0], 1e-5f);
                EXPECT_NEAR(scaleValues[i - 2][1], s[1], 1e-5f);
                EXPECT_NEAR(scaleValues[i - 2][2], s[2], 1e-5f);
            }

            // no retest with a time scalar of 2
            timeScalarPlug.setValue(2.0);
            MTime zeroTime(0.0, MTime::uiUnit());
            timeOffsetPlug.setValue(zeroTime);
            for (int i = 0; i < 25; ++i) {
                MTime time(i, MTime::uiUnit());
                MGlobal::viewFrame(time);

                MObject oMatrix;
                wsmPlug.getValue(oMatrix);
                MFnMatrixData fnMatrix(oMatrix);
                fnMatrix.matrix();

                EXPECT_NEAR(transformMatrix->getTimeCode().GetValue(), i * 2.0, 1e-5f);

                MTime offsetTime = outTimePlug.asMTime();
                EXPECT_NEAR(offsetTime.value(), time.value() * 2.0, 1e-5f);

                MVector T = fnx.getTranslation(MSpace::kTransform);
                EXPECT_NEAR(translateValues[i * 2][0], T.x, 1e-5f);
                EXPECT_NEAR(translateValues[i * 2][1], T.y, 1e-5f);
                EXPECT_NEAR(translateValues[i * 2][2], T.z, 1e-5f);

                const float    degToRad = 3.141592654f / 180.0f;
                MEulerRotation rotation;
                fnx.getRotation(rotation);
                EXPECT_NEAR(degToRad * rotateValues[i * 2][0], rotation.x, 1e-5f);
                EXPECT_NEAR(degToRad * rotateValues[i * 2][1], rotation.y, 1e-5f);
                EXPECT_NEAR(degToRad * rotateValues[i * 2][2], rotation.z, 1e-5f);

                double s[3];
                fnx.getScale(s);
                EXPECT_NEAR(scaleValues[i * 2][0], s[0], 1e-5f);
                EXPECT_NEAR(scaleValues[i * 2][1], s[1], 1e-5f);
                EXPECT_NEAR(scaleValues[i * 2][2], s[2], 1e-5f);
            }
            timeScalarPlug.setValue(1.0);
        }

        if (MGlobal::kInteractive == MGlobal::mayaState())
            MGlobal::executeCommand("refresh -suspend true");
    }
}

// Test that both, ie, "translateTo" and "translateBy" methods work, for all
// xform ops
TEST(Transform, checkXformByAndTo)
{
    MStatus     status;
    const char* xformName = "myXform";
    SdfPath     xformPath(std::string("/") + xformName);

    auto constructTransformChain = [&]() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   a = UsdGeomXform::Define(stage, xformPath);
        return stage;
    };

    MFileIO::newFile(true);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_transform_checkXformByAndTo.usda");
    std::string       sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    {
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MString    proxyParentMayaPath = fn.fullPathName();
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
        MString    proxyShapeMayaPath = fn.fullPathName();

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        auto         xformPrim = stage->GetPrimAtPath(xformPath);
        UsdGeomXform xformGeom(xformPrim);

        // Make the xform in maya
        MString cmd = MString("select -r \"") + proxyShapeMayaPath
            + "\"; AL_usdmaya_ProxyShapeImportAllTransforms;";
        ASSERT_EQ(MS::kSuccess, MGlobal::executeCommand(cmd));

        MSelectionList sel;
        sel.add("myXform");
        MObject myXformObj;
        sel.getDependNode(0, myXformObj);
        ASSERT_FALSE(myXformObj.isNull());
        MFnTransform myXformMfn(myXformObj, &status);
        ASSERT_EQ(MS::kSuccess, status);

        MVector        expectedTranslation(0, 0, 0);
        MVector        expectedRotatePivotTranslation(0, 0, 0);
        MVector        expectedRotatePivot(0, 0, 0);
        MEulerRotation expectedRotation(0, 0, 0);
        MQuaternion    expectedOrientation;
        MVector        expectedScalePivotTranslation(0, 0, 0);
        MVector        expectedScalePivot(0, 0, 0);
        double         expectedShear[3] = { 0, 0, 0 };
        double         expectedScale[3] = { 1, 1, 1 };
        // Originally had this as an MTransformationMatrix for easy comparison, but
        // it seems there's a bug with MTransformationMatrix.setRotationOrientation - or, perhaps,
        // it always functions as though balance=true?
        MPxTransformationMatrix expectedMatrix;

        auto assertExpectedXform = [&]() {
            ASSERT_EQ(myXformMfn.getTranslation(MSpace::kTransform), expectedTranslation);
            ASSERT_EQ(
                myXformMfn.rotatePivotTranslation(MSpace::kTransform),
                expectedRotatePivotTranslation);
            ASSERT_EQ(myXformMfn.rotatePivot(MSpace::kTransform), expectedRotatePivot);
            MEulerRotation actualRotation;
            myXformMfn.getRotation(actualRotation);
            ASSERT_EQ(actualRotation, expectedRotation);
            ASSERT_TRUE(
                myXformMfn.rotateOrientation(MSpace::kTransform).isEquivalent(expectedOrientation));
            ASSERT_EQ(
                myXformMfn.scalePivotTranslation(MSpace::kTransform),
                expectedScalePivotTranslation);
            ASSERT_EQ(myXformMfn.scalePivot(MSpace::kTransform), expectedScalePivot);
            double actualShear[3];
            myXformMfn.getShear(actualShear);
            ASSERT_EQ(actualShear[0], expectedShear[0]);
            ASSERT_EQ(actualShear[1], expectedShear[1]);
            ASSERT_EQ(actualShear[2], expectedShear[2]);
            double actualScale[3];
            myXformMfn.getScale(actualScale);
            ASSERT_EQ(actualScale[0], expectedScale[0]);
            ASSERT_EQ(actualScale[1], expectedScale[1]);
            ASSERT_EQ(actualScale[2], expectedScale[2]);
            MMatrix expectedMMatrix = expectedMatrix.asMatrix();
            MMatrix actualMMatrix = myXformMfn.transformation().asMatrix();
            if (!expectedMMatrix.isEquivalent(expectedMMatrix, 1e-3)) {
                std::cout << "actualMatrix:" << std::endl;
                std::cout << actualMMatrix << std::endl;
                std::cout << "expectedMatrix:" << std::endl;
                std::cout << expectedMMatrix << std::endl;
                ASSERT_TRUE(false);
            }
        };

        {
            SCOPED_TRACE("inital empty xform");
            assertExpectedXform();
        }

        expectedTranslation = MVector(1, 2, 3);
        myXformMfn.setTranslation(expectedTranslation, MSpace::kTransform);
        expectedMatrix.translateTo(expectedTranslation, MSpace::kTransform);
        {
            SCOPED_TRACE("translateTo");
            assertExpectedXform();
        }
        myXformMfn.translateBy(MVector(4, 5, 6), MSpace::kTransform);
        expectedTranslation = MVector(5, 7, 9);
        expectedMatrix.translateTo(expectedTranslation, MSpace::kTransform);
        {
            SCOPED_TRACE("translateBy");
            assertExpectedXform();
        }

        expectedRotatePivotTranslation = MVector(.1, .2, .3);
        myXformMfn.setRotatePivotTranslation(expectedRotatePivotTranslation, MSpace::kTransform);
        expectedMatrix.setRotatePivotTranslation(
            expectedRotatePivotTranslation, MSpace::kTransform);
        {
            SCOPED_TRACE("rotatePivotTranslate");
            assertExpectedXform();
        }

        expectedRotatePivot = MVector(.9, .8, .7);
        myXformMfn.setRotatePivot(expectedRotatePivot, MSpace::kTransform, false);
        expectedMatrix.setRotatePivot(expectedRotatePivot, MSpace::kTransform, false);
        {
            SCOPED_TRACE("rotatePivot");
            assertExpectedXform();
        }

        expectedOrientation.setAxisAngle(MVector(2, 1, -5), .83);
        myXformMfn.setRotateOrientation(expectedOrientation, MSpace::kTransform, false);
        expectedMatrix.setRotateOrientation(expectedOrientation, MSpace::kTransform, false);
        {
            SCOPED_TRACE("rotateOrient");
            assertExpectedXform();
        }

        // 15/30/60 degrees, if you're curious
        expectedRotation.setValue(0.2617993877991494, 0.5235987755982988, 1.0471975511965976);
        myXformMfn.setRotation(expectedRotation);
        expectedMatrix.rotateTo(expectedRotation);
        {
            SCOPED_TRACE("rotateTo");
            assertExpectedXform();
        }
        // 8/7/6 degrees
        MEulerRotation addedRotate(0.13962634015954636, 0.12217304763960307, 0.10471975511965978);
        myXformMfn.rotateBy(addedRotate, MSpace::kTransform);
        // The euler rotations aren't simple additions - ie, not x+dx, y+dy, z+dz
        // instead, the two rotation matrices are multiplied... so for simplicity,
        // we just rely on MPxTransformationMatrix.rotateBy to get us the new
        // expected value
        expectedMatrix.rotateBy(addedRotate);
        expectedRotation = expectedMatrix.eulerRotation();
        {
            SCOPED_TRACE("rotateBy");
            assertExpectedXform();
        }

        expectedScalePivotTranslation = MVector(-.04, -.05, -.06);
        myXformMfn.setScalePivotTranslation(expectedScalePivotTranslation, MSpace::kTransform);
        expectedMatrix.setScalePivotTranslation(expectedScalePivotTranslation, MSpace::kTransform);
        {
            SCOPED_TRACE("scalePivotTranslate");
            assertExpectedXform();
        }

        expectedScalePivot = MVector(10, 20, 30);
        myXformMfn.setScalePivot(expectedScalePivot, MSpace::kTransform, false);
        expectedMatrix.setScalePivot(expectedScalePivot, MSpace::kTransform, false);
        {
            SCOPED_TRACE("scalePivot");
            assertExpectedXform();
        }

        expectedShear[0] = .4;
        expectedShear[1] = .5;
        expectedShear[2] = -.8;
        myXformMfn.setShear(expectedShear);
        expectedMatrix.shearTo(MVector(expectedShear), MSpace::kTransform);
        {
            SCOPED_TRACE("shearTo");
            assertExpectedXform();
        }
        double shearBy[3] = { 2, 3, 4 };
        myXformMfn.shearBy(shearBy);
        expectedShear[0] = .8;
        expectedShear[1] = 1.5;
        expectedShear[2] = -3.2;
        expectedMatrix.shearTo(MVector(expectedShear), MSpace::kTransform);
        {
            SCOPED_TRACE("shearBy");
            assertExpectedXform();
        }

        expectedScale[0] = 7;
        expectedScale[1] = 13;
        expectedScale[2] = 17;
        myXformMfn.setScale(expectedScale);
        expectedMatrix.scaleTo(MVector(expectedScale), MSpace::kTransform);
        {
            SCOPED_TRACE("scaleTo");
            assertExpectedXform();
        }
        double scaleBy[3] = { 5, -1, 2 };
        myXformMfn.scaleBy(scaleBy);
        expectedScale[0] = 35;
        expectedScale[1] = -13;
        expectedScale[2] = 34;
        expectedMatrix.scaleTo(MVector(expectedScale), MSpace::kTransform);
        {
            SCOPED_TRACE("scaleBy");
            assertExpectedXform();
        }
    }
}

//  inline UsdTimeCode getTimeCode()
//  void enableReadAnimatedValues(bool enabled);
//  inline bool readAnimatedValues() const
//  inline bool pushToPrimEnabled() const
//  void enablePushToPrim(bool enabled);
TEST(Transform, getTimeCode)
{
    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   a = UsdGeomXform::Define(stage, SdfPath("/tm"));
        return stage;
    };

    MFileIO::newFile(true);

    // In 'off' (DG) mode, setCurrentTime does not seem to trigger an eval.
    // Force it to 'parallel' for now.
    MGlobal::executeCommand(MString("evaluationManager -mode \"parallel\";"));

    const std::string temp_path = buildTempPath("AL_USDMayaTests_transform_getTimeCode.usda");
    std::string       sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }
    MGlobal::viewFrame(-10);

    MString shapeName;
    {
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        {
            MGlobal::executeCommand(
                MString("connectAttr -f \"time1.outTime\" \"") + fn.name() + ".time\";");
        }

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        MDagModifier modifier1;
        MDGModifier  modifier2;

        // construct a chain of transform nodes
        MObject leafNode = proxy->makeUsdTransforms(
            stage->GetPrimAtPath(SdfPath("/tm")),
            modifier1,
            AL::usdmaya::nodes::ProxyShape::kRequested,
            &modifier2);

        // make sure we get some sane looking values.
        EXPECT_FALSE(leafNode == MObject::kNullObj);
        EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
        EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());

        MFnTransform                   fnx(leafNode);
        AL::usdmaya::nodes::Transform* transformNode
            = (AL::usdmaya::nodes::Transform*)fnx.userNode();

        AL::usdmaya::nodes::TransformationMatrix* transformMatrix = transformNode->getTransMatrix();

        transformNode->pushToPrimPlug().setValue(false);
        transformNode->readAnimatedValuesPlug().setValue(false);
        EXPECT_FALSE(transformMatrix->pushToPrimEnabled());
        EXPECT_FALSE(transformMatrix->readAnimatedValues());

        // if we don't re-enable the refresh for this test, the scene won't get updated when calling
        // view frame
        if (MGlobal::kInteractive == MGlobal::mayaState())
            MGlobal::executeCommand("refresh -suspend false");

        EXPECT_EQ(UsdTimeCode::Default(), transformMatrix->getTimeCode());

        transformNode->pushToPrimPlug().setValue(false);
        transformNode->readAnimatedValuesPlug().setValue(true);
        EXPECT_FALSE(transformMatrix->pushToPrimEnabled());
        EXPECT_TRUE(transformMatrix->readAnimatedValues());

        MTime time(42, MTime::uiUnit());
        MAnimControl::setCurrentTime(time);
        MAnimControl::setCurrentTime(time);

        EXPECT_EQ(UsdTimeCode(42), transformMatrix->getTimeCode());

        if (MGlobal::kInteractive == MGlobal::mayaState())
            MGlobal::executeCommand("refresh -suspend true");
    }
}

// Need to test the behaviour of the transform node when the animation data present is from Matrices
// rather than TRS components.
TEST(Transform, matrixAnimationChannels) { AL_USDMAYA_UNTESTED; }

// Test twisted rotation values (angles should be considered the same)
TEST(Transform, checkTwistedRotation)
{
    MStatus     status;
    const char* xformName = "myXform";
    SdfPath     xformPath(std::string("/") + xformName);
    SdfPath     spherePath(xformPath.AppendChild(TfToken("mesh")));

    MFileIO::newFile(true);

    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_transform_checkTwistedRotation.usda");

    // generate some data for the proxy shape
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   a = UsdGeomXform::Define(stage, xformPath);
        auto           op = a.AddRotateXYZOp();
        op.Set(GfVec3f(180.f, -2.317184f, 180.f));
        UsdGeomSphere::Define(stage, spherePath);
        stage->Export(temp_path, false);
    }

    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MString    proxyParentMayaPath = fn.fullPathName();
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
    MString    proxyShapeMayaPath = fn.fullPathName();

    auto* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());

    auto stage = proxy->getUsdStage();

    // Verify current edit target has nothing
    ASSERT_TRUE(stage->GetEditTarget().GetLayer()->IsEmpty());

    MDagModifier modifier1;
    MDGModifier  modifier2;
    proxy->makeUsdTransformChain(
        stage->GetPrimAtPath(spherePath),
        modifier1,
        AL::usdmaya::nodes::ProxyShape::kSelection,
        &modifier2);
    EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
    EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());

    MSelectionList sel;
    sel.add("myXform");
    MObject xformMobj;
    ASSERT_TRUE(sel.getDependNode(0, xformMobj));
    MFnTransform xformMfn(xformMobj);

    {
        // Verify default values from Maya after loading USD
        MEulerRotation rot;
        ASSERT_TRUE(xformMfn.getRotation(rot) == MStatus::kSuccess);
        EXPECT_NEAR(rot.x, 0.f, 1e-5f);
        EXPECT_NEAR(rot.y, 3.1820349693298f, 1e-5f);
        EXPECT_NEAR(rot.z, 0.f, 1e-5f);
        // Expect nothing changed in USD
        ASSERT_TRUE(stage->GetEditTarget().GetLayer()->IsEmpty());
    }

    {
        // Explicitly rotate X axis by 360 degree, there should be no "over" on USD
        // Notice that we only set the X component here
        MPlug(xformMobj, MPxTransform::rotateX).setValue(M_PI * 2);
        ASSERT_TRUE(stage->GetEditTarget().GetLayer()->IsEmpty());
    }

    {
        auto                                     xformPrim = stage->GetPrimAtPath(xformPath);
        AL::usdmaya::nodes::TransformationMatrix tm;
        tm.setPrim(xformPrim, nullptr);

        MEulerRotation rot(M_PI, -2.317184f * M_PI / 180.f, M_PI);
        ((MPxTransformationMatrix*)(&tm))->rotateTo(rot);
        // Verify the matrix has been set to expected values
        MStatus status;
        auto    tmRot = tm.eulerRotation(MSpace::kTransform, &status);
        ASSERT_TRUE(status == MStatus::kSuccess);
        EXPECT_NEAR(tmRot.x, M_PI, 1e-5f);
        EXPECT_NEAR(tmRot.y, -2.317184f * M_PI / 180.f, 1e-5f);
        EXPECT_NEAR(tmRot.z, M_PI, 1e-5f);
        // Verify the USD rotation
        {
            GfVec3f usdRot { 0.f, 0.f, 0.f };
            ASSERT_TRUE(xformPrim.GetAttribute(TfToken("xformOp:rotateXYZ")).Get(&usdRot));
            EXPECT_NEAR(usdRot[0], 180.f, 1e-5f);
            EXPECT_NEAR(usdRot[1], -2.317184f, 1e-5f);
            EXPECT_NEAR(usdRot[2], 180.f, 1e-5f);
        }

        // Attempt to apply the rotation to USD
        tm.pushRotateToPrim();
        // Expect no "over" being created
        ASSERT_TRUE(stage->GetEditTarget().GetLayer()->IsEmpty());
        // Verify again the rotation values in USD
        {
            GfVec3f usdRot { 0.f, 0.f, 0.f };
            ASSERT_TRUE(xformPrim.GetAttribute(TfToken("xformOp:rotateXYZ")).Get(&usdRot));
            EXPECT_NEAR(usdRot[0], 180.f, 1e-5f);
            EXPECT_NEAR(usdRot[1], -2.317184f, 1e-5f);
            EXPECT_NEAR(usdRot[2], 180.f, 1e-5f);
        }
    }

    {
        auto                                     xformPrim = stage->GetPrimAtPath(xformPath);
        AL::usdmaya::nodes::TransformationMatrix tm;
        tm.setPrim(stage->GetPrimAtPath(xformPath), nullptr);

        MEulerRotation rot(0.f, 0.f, 0.f);
        ((MPxTransformationMatrix*)(&tm))->rotateTo(rot);

        // Verify the original rotation in USD
        {
            GfVec3f usdRot { 0.f, 0.f, 0.f };
            ASSERT_TRUE(xformPrim.GetAttribute(TfToken("xformOp:rotateXYZ")).Get(&usdRot));
            EXPECT_NEAR(usdRot[0], 180.f, 1e-5f);
            EXPECT_NEAR(usdRot[1], -2.317184f, 1e-5f);
            EXPECT_NEAR(usdRot[2], 180.f, 1e-5f);
        }
        // This should change the USD since the rotation values are different now
        tm.pushRotateToPrim();

        // Expect an "over" in USD
        ASSERT_FALSE(stage->GetEditTarget().GetLayer()->IsEmpty());
        auto primSpec = stage->GetEditTarget().GetLayer()->GetPrimAtPath(xformPath);
        ASSERT_TRUE(primSpec);
        EXPECT_EQ(primSpec->GetSpecifier(), SdfSpecifierOver);
        // Rotation should have been changed as well
        {
            GfVec3f usdRot { 0.f, 0.f, 0.f };
            ASSERT_TRUE(xformPrim.GetAttribute(TfToken("xformOp:rotateXYZ")).Get(&usdRot));
            EXPECT_NEAR(usdRot[0], 0.f, 1e-5f);
            EXPECT_NEAR(usdRot[1], 0.f, 1e-5f);
            EXPECT_NEAR(usdRot[2], 0.f, 1e-5f);
        }
    }
}
