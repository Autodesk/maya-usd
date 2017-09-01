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
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/proxy/PrimFilter.h"
#include "AL/usdmaya/StageCache.h"
#include "maya/MFnTransform.h"
#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MDagModifier.h"
#include "maya/MFileIO.h"
#include "maya/MStringArray.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

#include <fstream>


static const char* const g_drivenData =
"#usda 1.0\n"
"\n"
"def XForm \"root\"\n"
"{\n"
"    def XForm \"hip1\"\n"
"    {\n"
"        def XForm \"knee1\"\n"
"        {\n"
"            def XForm \"ankle1\"\n"
"            {\n"
"                def XForm \"ltoe1\"\n"
"                {\n"
"                }\n"
"            }\n"
"        }\n"
"    }\n"
"}\n";

//  DrivenTransforms();
//  size_t transformCount() const;
//  void initTransform(uint32_t index);
//  void constructDrivenPrimsArray(SdfPathVector& drivenPaths, std::vector<UsdPrim>& drivenPrims, UsdStageRefPtr stage);
//  void update(std::vector<UsdPrim>& drivenPrims, const MTime& currentTime);
//  void dirtyVisibility(int32_t primIndex, bool newValue);
//  void dirtyMatrix(int32_t primIndex, MMatrix newValue);
//  void setDrivenPrimPaths(const SdfPathVector& primPaths);
//  void visibilityReserve(uint32_t visibilityCount);
//  void matricesReserve(uint32_t matrixCount);
//  const SdfPathVector& drivenPrimPaths() const;
//  const std::vector<int32_t>& dirtyMatrices() const;
//  const std::vector<int32_t>& dirtyVisibilities() const;
//  const std::vector<MMatrix>& drivenMatrices() const;

TEST(ProxyShape, DrivenTransforms)
{
  AL::usdmaya::nodes::proxy::DrivenTransforms dt;

  // ensure constructor leaves arrays empty
  EXPECT_TRUE(dt.drivenPrimPaths().empty());
  EXPECT_TRUE(dt.dirtyMatrices().empty());
  EXPECT_TRUE(dt.dirtyVisibilities().empty());
  EXPECT_TRUE(dt.drivenMatrices().empty());
  EXPECT_EQ(0, dt.transformCount());

  const std::string temp_path = "/tmp/AL_USDMayaTests_proxy_DrivenTransforms.usda";
  std::string sessionLayerContents;

  // generate some data for the proxy shape
  {
    std::ofstream os(temp_path);
    os << g_drivenData;
  }

  MString shapeName;
  {
    MFnDagNode fn;
    MObject xform = fn.create("transform");
    MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());

    auto stage = proxy->getUsdStage();

    const SdfPathVector drivenPaths =
    {
      SdfPath("/root"),
      SdfPath("/root/hip1"),
      SdfPath("/root/hip1/knee1"),
      SdfPath("/root/hip1/knee1/ankle1"),
      SdfPath("/root/hip1/knee1/ankle1/ltoe1")
    };

    // initialising the transforms should result in prim paths being allocated,
    dt.initTransforms(drivenPaths.size());
    EXPECT_EQ(drivenPaths.size(), dt.drivenPrimPaths().size());
    EXPECT_EQ(drivenPaths.size(), dt.drivenVisibilities().size());
    EXPECT_EQ(drivenPaths.size(), dt.drivenMatrices().size());
    for(size_t i = 0; i < drivenPaths.size(); ++i)
    {
      EXPECT_EQ(std::string(), dt.drivenPrimPaths()[i].GetString());
      EXPECT_EQ(MMatrix::identity, dt.drivenMatrices()[i]);
      EXPECT_TRUE(dt.drivenVisibilities()[i]);
    }

    // ensure we can update the prim paths on the driven transforms object
    dt.setDrivenPrimPaths(drivenPaths);
    EXPECT_EQ(drivenPaths.size(), dt.drivenPrimPaths().size());
    for(size_t i = 0; i < drivenPaths.size(); ++i)
    {
      EXPECT_EQ(drivenPaths[i].GetString(), dt.drivenPrimPaths()[i].GetString());
    }

    // test to make sure that constructDrivenPrimsArray correctly initialises the returned arrays of paths and prims
    std::vector<UsdPrim> drivenPrims;
    dt.constructDrivenPrimsArray(drivenPrims, stage);
    for(size_t i = 0; i < drivenPaths.size(); ++i)
    {
      EXPECT_TRUE(drivenPrims[i].IsValid());
      EXPECT_EQ(drivenPaths[i].GetString(), drivenPrims[i].GetPath().GetString());
    }

    // make sure nothing has happened to the dirty visibility array in the previous code
    EXPECT_TRUE(dt.dirtyVisibilities().empty());

    // Make sure that when we dirty visibility with index 3, that the correct dirty flags are set
    dt.dirtyVisibility(3, false);

    // we should now have an index is the visibility array
    EXPECT_EQ(1, dt.dirtyVisibilities().size());
    EXPECT_EQ(3, dt.dirtyVisibilities()[0]);

    // the corresponding element in the visibilities should be set to false, the rest should remain true
    for(size_t i = 0; i < drivenPaths.size(); ++i)
    {
      if(i != 3)
      {
        EXPECT_TRUE(dt.drivenVisibilities()[i]);
      }
      else
      {
        EXPECT_FALSE(dt.drivenVisibilities()[i]);
      }
    }

    // test to make sure that updateDrivenVisibility adds a keyframe value in the visibility data for
    MTime time(10.0f, MTime::uiUnit());
    dt.update(drivenPrims, time);

    for(size_t i = 0; i < drivenPrims.size(); ++i)
    {
      UsdGeomXform xform(drivenPrims[i]);
      UsdAttribute attr = xform.GetVisibilityAttr();

      if(i != 3)
      {
        EXPECT_FALSE(attr.HasValue());
      }
      else
      {
        EXPECT_TRUE(attr.HasValue());
        TfToken token;
        attr.Get(&token, 10.0f);
        EXPECT_TRUE(UsdGeomTokens->invisible == token);
      }
    }

    // After the visibilites have been updated, the dirtyVisibilities should have been cleared.
    EXPECT_EQ(0, dt.dirtyVisibilities().size());

    //  Check to see that dirtyMatrix initialises the correct indices and transform values in the DrivenTransforms object
    MMatrix matrixValue = MMatrix::identity;
    matrixValue[3][0] = 3.0;
    dt.dirtyMatrix(2, matrixValue);

    // we should now have an index is the visibility array
    EXPECT_EQ(1, dt.dirtyMatrices().size());
    EXPECT_EQ(2, dt.dirtyMatrices()[0]);

    for(size_t i = 0; i < drivenPaths.size(); ++i)
    {
      if(i != 2)
      {
        EXPECT_TRUE(dt.drivenMatrices()[i] == MMatrix::identity);
      }
      else
      {
        EXPECT_EQ(dt.drivenMatrices()[i], matrixValue);
      }
    }

    // test to make sure that updateDrivenVisibility adds a keyframe value in the visibility data for
    dt.update(drivenPrims, time);

    for(size_t i = 0; i < drivenPrims.size(); ++i)
    {
      UsdGeomXform xform(drivenPrims[i]);
      bool resetsXformStack;
      std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&resetsXformStack);

      if(i != 2)
      {
        EXPECT_TRUE(ops.empty());
      }
      else
      {
        EXPECT_FALSE(ops.empty());
        EXPECT_TRUE(ops[0].GetOpType() == UsdGeomXformOp::TypeTransform);
        MMatrix returnedMatrix;
        ops[0].Get((GfMatrix4d*)&returnedMatrix, 10.0);
        EXPECT_EQ(matrixValue, returnedMatrix);
      }
    }

  }
}
