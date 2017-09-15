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

#include "maya/MFnDependencyNode.h"
#include "maya/MFnDagNode.h"
#include "maya/MFnTransform.h"
#include "maya/MFileIO.h"
#include "maya/MDGModifier.h"

#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/HostDrivenTransforms.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

TEST(HostDrivenTransforms, basicDrivenTransforms)
{
  MFileIO::newFile(true);

  auto constructHostDrivenTransforms = [] (std::vector<UsdGeomXform>& xforms)
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform cube = UsdGeomXform::Define(stage, SdfPath("/root/cube"));

    xforms.push_back(root);
    xforms.push_back(cube);
    return stage;
  };

  std::vector<UsdGeomXform> xforms;
  const std::string temp_path = "/tmp/AL_USDMayaTests_basicDrivenTransforms.usda";
  std::string seesionLayerContents;

  // generate some data for the proxy shape
  {
    auto stage = constructHostDrivenTransforms(xforms);
    stage->Export(temp_path, false);
  }

  // create and connect proxy shape and host driven transform
  {
    MFnDagNode fnDag;
    MObject xform = fnDag.create("transform");
    MObject shape = fnDag.create("AL_usdmaya_ProxyShape", xform);
    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fnDag.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());
    auto stage = proxy->getUsdStage();

    MPlug inDataPlug = proxy->inDrivenTransformsDataPlug();
    inDataPlug.setNumElements(1);
    MPlug inData = inDataPlug.elementByLogicalIndex(0);

    MFnDependencyNode fnDG;
    MObject drivenTransforms = fnDG.create("AL_usdmaya_HostDrivenTransforms");
    AL::usdmaya::nodes::HostDrivenTransforms* driven = (AL::usdmaya::nodes::HostDrivenTransforms*)fnDG.userNode();
    MPlug primPathsPlug = driven->drivenPrimPathsPlug();
    primPathsPlug.setNumElements(1);
    MPlug primPath = primPathsPlug.elementByLogicalIndex(0);
    primPath.setString("/root/cube");
    MPlug drivenTranslatePlug = driven->drivenTranslatePlug();
    drivenTranslatePlug.setNumElements(1);
    MPlug drivenTranslate = drivenTranslatePlug.elementByLogicalIndex(0);
    MPlug outData = driven->outDrivenTransformsDataPlug();

    MObject hostTransform = fnDag.create("transform");
    MFnTransform fnTrans(hostTransform);
    fnTrans.setTranslation(MVector(1.0, 2.0, 3.0), MSpace::kTransform);
    MPlug hostTranslate = fnDag.findPlug("translate");

    MDGModifier dgMod;
    dgMod.connect(outData, inData);
    dgMod.connect(hostTranslate, drivenTranslate);
    dgMod.doIt();

    // check result
    {
      auto stage = proxy->getUsdStage();

      UsdPrim prim = stage->GetPrimAtPath(SdfPath("/root/cube"));
      EXPECT_TRUE(prim.IsValid());
      UsdGeomXform xform(prim);
      bool resetsXformStack = false;
      std::vector<UsdGeomXformOp> xformops = xform.GetOrderedXformOps(&resetsXformStack);
      EXPECT_EQ(1, xformops.size());
      if(!xformops.empty())
      {
        UsdGeomXformOp xop = xformops[0];
        GfMatrix4d matrix = xop.GetOpTransform(0);
        GfVec3d translate = matrix.ExtractTranslation();
        EXPECT_DOUBLE_EQ(translate[0], 1.0);
        EXPECT_DOUBLE_EQ(translate[1], 2.0);
        EXPECT_DOUBLE_EQ(translate[2], 3.0);
      }
    }
  }
}

