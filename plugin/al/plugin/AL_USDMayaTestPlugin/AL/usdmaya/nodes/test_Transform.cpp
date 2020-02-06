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
#include "test_usdmaya.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/Scope.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"
#include "maya/MAnimControl.h"
#include "maya/MFileIO.h"
#include "maya/MFnDagNode.h"
#include "maya/MFnTransform.h"
#include "maya/MGlobal.h"
#include "maya/MPlug.h"
#include "maya/MStatus.h"
#include "maya/MTypes.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/xformable.h"

using AL::usdmaya::nodes::ProxyShape;
using AL::usdmaya::nodes::Transform;
using AL::usdmaya::nodes::Scope;
using AL::usdmaya::nodes::BasicTransformationMatrix;
using AL::usdmaya::nodes::TransformationMatrix;

using AL::maya::test::buildTempPath;


// Check that we can set various values in an AL_USDMaya Transform and have USD reflect them
TEST(Transform, noInputStage)
{
  MStatus status;
  MFileIO::newFile(true);

  MFnDagNode dagFn;
  MObject xform = dagFn.create(Transform::kTypeId);
  MFnTransform transFn(xform);
  Transform* ptrXform = (Transform*)transFn.userNode();
  TransformationMatrix* ptrMatrix = ptrXform->getTransMatrix();

  MPlug pushToPrimPlug = ptrXform->pushToPrimPlug();
  EXPECT_FALSE(pushToPrimPlug.asBool());
  EXPECT_FALSE(ptrMatrix->pushToPrimEnabled());
  EXPECT_FALSE(ptrMatrix->pushToPrimAvailable());

  auto checkTranslation = [&](double x, double y, double z) {
    MVector transOut = transFn.getTranslation(MSpace::kObject, &status);
    EXPECT_EQ(MS::kSuccess, status);
    EXPECT_EQ(x, transOut.x);
    EXPECT_EQ(y, transOut.y);
    EXPECT_EQ(z, transOut.z);
  };

  auto setAndCheckTranslation = [&](double x, double y, double z) {
    MVector transIn;
    transIn.x = x;
    transIn.y = y;
    transIn.z = z;
    EXPECT_EQ(MS::kSuccess, transFn.setTranslation(transIn, MSpace::kObject));
    checkTranslation(x, y, z);
  };

  checkTranslation(0.0, 0.0, 0.0);
  setAndCheckTranslation(1.0, 2.0, 3.0);

  pushToPrimPlug.setBool(true);
  EXPECT_TRUE(pushToPrimPlug.asBool());
  EXPECT_TRUE(ptrMatrix->pushToPrimEnabled());
  EXPECT_FALSE(ptrMatrix->pushToPrimAvailable());

  setAndCheckTranslation(4.0, 5.0, 6.0);
}

// Check that we can set various values in an AL_USDMaya Scope and have USD reflect them (or oot)
TEST(Scope, noInputStage)
{

  MStatus status;
  MFileIO::newFile(true);

  MFnDagNode dagFn;
  MObject xform = dagFn.create(Scope::kTypeId);
  MFnTransform transFn(xform);
  Scope* ptrXform = (Scope*)transFn.userNode();
  BasicTransformationMatrix* ptrMatrix = ptrXform->transform();

  EXPECT_FALSE(ptrMatrix->pushToPrimAvailable());

  auto checkTranslation = [&](double x, double y, double z) {
    MVector transOut = transFn.getTranslation(MSpace::kObject, &status);
    EXPECT_EQ(MS::kSuccess, status);
    EXPECT_EQ(x, transOut.x);
    EXPECT_EQ(y, transOut.y);
    EXPECT_EQ(z, transOut.z);
  };

  checkTranslation(0.0, 0.0, 0.0);
  MVector transIn;
  transIn.x = 1.0;
  transIn.y = 2.0;
  transIn.z = 3.0;
  transFn.setTranslation(transIn, MSpace::kObject);
  checkTranslation(0.0, 0.0, 0.0);

}

// Make sure animation data isn't lost when a Transform node exists
// (Was a bug where m_time was incorrectly getting reset to default, resulting
// in animation data not being read correctly)
TEST(Transform, animationWithTransform)
{
  MStatus status;
  MFileIO::newFile(true);
  MGlobal::viewFrame(1);

  int optionVarValue = MGlobal::optionVarIntValue("AL_usdmaya_readAnimatedValues");
  MGlobal::setOptionVarValue("AL_usdmaya_readAnimatedValues", true);

//  MString importCommand = "AL_usdmaya_ProxyShapeImport -connectToTime 1 -f \"" +
  MString importCommand = "AL_usdmaya_ProxyShapeImport -f \"" +
                          MString(AL_USDMAYA_TEST_DATA) +
                          "/cube_moving_zaxis.usda\"";

  MStringArray cmdResults;
  status = MGlobal::executeCommand(importCommand, cmdResults, true);
  ASSERT_TRUE(status == MStatus::kSuccess);
  MString proxyName = cmdResults[0];

  MString selectCommand = "AL_usdmaya_ProxyShapeSelect -primPath \"/pCube1\" -proxy \"" + proxyName + "\"";
  cmdResults.clear();
  status = MGlobal::executeCommand(selectCommand, cmdResults, true);
  ASSERT_TRUE(status == MStatus::kSuccess);
  MString xformName = cmdResults[0];

  MSelectionList sel;
  sel.add(proxyName);
  sel.add(xformName);
  MDagPath proxyDagPath;
  MDagPath xformDagPath;
  sel.getDagPath(0, proxyDagPath);
  sel.getDagPath(1, xformDagPath);
  MFnDagNode proxyMFn(proxyDagPath, &status);
  ASSERT_TRUE(status == MStatus::kSuccess);
  MFnDagNode xformMFn(xformDagPath, &status);
  ASSERT_TRUE(status == MStatus::kSuccess);

  auto proxy = dynamic_cast<ProxyShape*>(proxyMFn.userNode(&status));
  ASSERT_TRUE(status == MStatus::kSuccess);

  auto stage = proxy->getUsdStage();
  ASSERT_TRUE(stage);
  auto prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
  ASSERT_TRUE(prim.IsValid());

  UsdGeomXformable xformable(prim);
  ASSERT_TRUE(xformable);

  // Make sure the time attrs are hooked up properly
  ASSERT_FALSE(proxyMFn.findPlug("time").source().isNull());
  ASSERT_FALSE(xformMFn.findPlug("time").source().isNull());

  const auto origin = GfVec4d(0, 0, 0, 1);

  auto assertTranslate = [&xformMFn, &origin, &xformable]
                          (double expectedX, double expectedY, double expectedZ) {

    ASSERT_FLOAT_EQ(xformMFn.findPlug("translateX").asDouble(), expectedX);
    ASSERT_FLOAT_EQ(xformMFn.findPlug("translateY").asDouble(), expectedY);
    ASSERT_FLOAT_EQ(xformMFn.findPlug("translateZ").asDouble(), expectedZ);

    GfMatrix4d transform;
    bool resetsXform;
    xformable.GetLocalTransformation(&transform, &resetsXform,
        UsdTimeCode(MAnimControl::currentTime().value()));
    GfVec4d xformedPos = origin * transform;

    ASSERT_FLOAT_EQ(xformedPos[0], expectedX);
    ASSERT_FLOAT_EQ(xformedPos[1], expectedY);
    ASSERT_FLOAT_EQ(xformedPos[2], expectedZ);
    ASSERT_FLOAT_EQ(xformedPos[3], 1.0);
  };

  { SCOPED_TRACE(""); assertTranslate(0.0, 2.0, 0.0); }

  MGlobal::viewFrame(10);
  { SCOPED_TRACE(""); assertTranslate(0.0, 2.0, -6.7904988904413575); }

  MGlobal::viewFrame(24);
  { SCOPED_TRACE(""); assertTranslate(0.0, 2.0, -20.0); }

  MGlobal::setOptionVarValue("AL_usdmaya_readAnimatedValues", optionVarValue);
}


