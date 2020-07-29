//
// Copyright 2019 Animal Logic
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
#include "AL/usdmaya/nodes/Scope.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"
#include "test_usdmaya.h"

#include <pxr/usd/usdGeom/xform.h>

#include <maya/MFileIO.h>
#include <maya/MFnMatrixData.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MString.h>

using AL::maya::test::buildTempPath;

// Check that mixing AL_USDMaya Transforms and Scopes gives us the right answers
TEST(Scope, checkWorksInChain)
{
    auto constructTransformChain = []() {
        GfVec3d        v3d0(2.0f, 3.0f, 4.0f);
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   a0 = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomScope   a1 = UsdGeomScope::Define(stage, SdfPath("/root/scope1"));
        UsdGeomXform   a2 = UsdGeomXform::Define(stage, SdfPath("/root/scope1/xform1"));
        UsdGeomScope   a3 = UsdGeomScope::Define(stage, SdfPath("/root/scope1/xform1/scope2"));
        auto op1 = a2.AddTranslateOp(UsdGeomXformOp::PrecisionDouble, TfToken("translate"));
        op1.Set(v3d0);
        return stage;
    };

    auto getWorldMatrixTranslate = [&](const MFnDependencyNode& dag, MObject obj) {
        MVector trans;
        MObject worldMatrixAttr = dag.attribute("worldMatrix");
        MPlug   matrixPlug(obj, worldMatrixAttr);
        matrixPlug = matrixPlug.elementByLogicalIndex(0);

        // Get the value of the 'worldMatrix' attribute
        //
        MObject matrixObject;
        MStatus status = matrixPlug.getValue(matrixObject);
        if (!status) {
            return trans;
        }
        MFnMatrixData worldMatrixData(matrixObject, &status);
        if (!status) {
            return trans;
        }
        MMatrix worldMatrix = worldMatrixData.matrix(&status);
        if (!status) {
            return trans;
        }
        trans[0] = worldMatrix(3, 0);
        trans[1] = worldMatrix(3, 1);
        trans[2] = worldMatrix(3, 2);
        return trans;
    };

    MFileIO::newFile(true);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_scope_checkWorksInChain.usda");
    std::string       sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

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
        MFnTransform      xform(obj);

        MString                    str;
        AL::usdmaya::nodes::Scope* ptr = (AL::usdmaya::nodes::Scope*)fn.userNode();

        if (ptr->typeId() == AL::usdmaya::nodes::Transform::kTypeId) {
            // It's a transform
            str = ptr->primPathPlug().asString();

            if (str == "/root") {
                MVector expectedTranslation(0, 0, 0);
                MVector worldMatrixTranslation = getWorldMatrixTranslate(fn, obj);
                ASSERT_EQ(expectedTranslation, expectedTranslation);
            } else if (str == "/root/scope1/xform1") {

                MVector expectedTranslation(2, 3, 4);
                MVector worldMatrixTranslation = getWorldMatrixTranslate(fn, obj);
                ASSERT_EQ(expectedTranslation, expectedTranslation);
            }
        } else if (ptr->typeId() == AL::usdmaya::nodes::Scope::kTypeId) {
            // It's a scope
            str = ptr->primPathPlug().asString();

            if (str == "/root/scope1") {
                MVector expectedTranslation(0, 0, 0);
                MVector worldMatrixTranslation = getWorldMatrixTranslate(fn, obj);
                ASSERT_EQ(expectedTranslation, expectedTranslation);
            } else if (str == "/root/scope1/xform1/scope2") {
                MVector expectedTranslation(2, 3, 4);
                MVector worldMatrixTranslation = getWorldMatrixTranslate(fn, obj);
                ASSERT_EQ(expectedTranslation, expectedTranslation);
            }
        }
    }
}
