//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
#include "AL/usdmaya/nodes/Transform.h"
#include "maya/MFileIO.h"
#include "maya/MFnDagNode.h"
#include "maya/MFnTransform.h"
#include "maya/MPlug.h"
#include "maya/MStatus.h"
#include "maya/MTypes.h"

#include "pxr/usd/usd/stage.h"

using AL::usdmaya::nodes::Transform;

TEST(Transform, noInputStage)
{
  MStatus status;
  MFileIO::newFile(true);

  MFnDagNode dagFn;
  MObject xform = dagFn.create(Transform::kTypeId);
  MFnTransform transFn(xform);
  Transform* ptrXform = (Transform*)transFn.userNode();

  MPlug pushToPrimPlug = ptrXform->pushToPrimPlug();
  EXPECT_EQ(false, pushToPrimPlug.asBool());

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
  EXPECT_EQ(true, pushToPrimPlug.asBool());

  setAndCheckTranslation(4.0, 5.0, 6.0);
}
