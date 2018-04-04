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
#include "maya/MFnTransform.h"
#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MFileIO.h"

TEST(ProxyShapeSelect, selectNode)
{
  MFileIO::newFile(true);
  // unsure undo is enabled for this test
  MGlobal::executeCommand("undoInfo -state 1;");

  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    UsdGeomXform knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
    UsdGeomXform ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
    UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
    UsdGeomXform ltoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/rtoe1"));
    UsdGeomXform leg2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2"));
    UsdGeomXform knee2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2"));
    UsdGeomXform ankle2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2"));
    UsdGeomXform rtoe2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2/ltoe2"));
    UsdGeomXform ltoe2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2/rtoe2"));

    return stage;
  };

  auto compareNodes = [] (const SdfPathVector& paths)
  {
    MSelectionList sl;
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(sl.length(), paths.size());
    for(uint32_t i = 0; i < sl.length(); ++i)
    {
      MObject obj;
      sl.getDependNode(i, obj);
      MFnDependencyNode fn(obj);
      MStatus status;
      MPlug plug = fn.findPlug("primPath", &status);
      EXPECT_EQ(MStatus(MS::kSuccess), status);
      MString pathName = plug.asString();
      bool found = false;
      for(uint32_t j = 0; j < paths.size(); ++j)
      {
        if(pathName == paths[j].GetText())
        {
          found = true;
          break;
        }
      }
      EXPECT_TRUE(found);
    }
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_selectNode.usda";
  std::string sessionLayerContents;


  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MFnDagNode fn;
  MObject xform = fn.create("transform");
  MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);
  MString shapeName = fn.name();

  AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

  // force the stage to load
  proxy->filePathPlug().setString(temp_path.c_str());

  // select a single path
  MGlobal::executeCommand("select -cl;");
  MStringArray results;
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" \"AL_usdmaya_ProxyShape1\"", results, false, true);
  MSelectionList sl;

  EXPECT_EQ(1, results.length());
  EXPECT_EQ(MString("|transform1|root|hip1|knee1|ankle1|ltoe1"), results[0]);
  results.clear();

  // make sure the path is contained in the selected paths (for hydra selection)
  EXPECT_EQ(1, proxy->selectedPaths().size());
  EXPECT_EQ(1, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip1/knee1/ankle1/ltoe1")});


  // make sure undo clears the previous info
  MGlobal::executeCommand("undo", false, true);
  EXPECT_EQ(0, proxy->selectedPaths().size());
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(0, sl.length());


  // make sure redo works happily without side effects
  MGlobal::executeCommand("redo", false, true);

  EXPECT_EQ(1, proxy->selectedPaths().size());
  EXPECT_EQ(1, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip1/knee1/ankle1/ltoe1")});

  // So now we have a single item selected. Let's see if we can replace this selection list
  // with two other paths. The previous selection should be removed, the two additional paths
  // should be selected
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip2/knee2/ankle2/ltoe2\" -pp \"/root/hip2/knee2/ankle2/rtoe2\" \"AL_usdmaya_ProxyShape1\"", results, false, true);
  EXPECT_EQ(2, results.length());
  EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|ltoe2"), results[0]);
  EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|rtoe2"), results[1]);
  results.clear();

  EXPECT_EQ(2, proxy->selectedPaths().size());
  EXPECT_EQ(1, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_EQ(1, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  // when undoing this command, the previous path should be selected
  MGlobal::executeCommand("undo", false, true);

  EXPECT_EQ(1, proxy->selectedPaths().size());
  EXPECT_EQ(1, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  compareNodes({SdfPath("/root/hip1/knee1/ankle1/ltoe1")});

  MGlobal::executeCommand("redo", false, true);

  EXPECT_EQ(2, proxy->selectedPaths().size());
  EXPECT_EQ(1, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_EQ(1, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  // now attempt to clear the selection list
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -cl \"AL_usdmaya_ProxyShape1\"", results, false, true);
  EXPECT_EQ(0, results.length());

  EXPECT_EQ(0, proxy->selectedPaths().size());
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(0, sl.length());

  // undoing this command should return selected items back into
  MGlobal::executeCommand("undo", false, true);

  EXPECT_EQ(2, proxy->selectedPaths().size());
  EXPECT_EQ(1, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_EQ(1, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  MGlobal::executeCommand("redo", false, true);

  EXPECT_EQ(0, proxy->selectedPaths().size());
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(0, sl.length());


  // So now we have a single item selected. Let's see if we can replace this selection list
  // with two other paths. The previous selection should be removed, the two additional paths
  // should be selected
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -a -pp \"/root/hip2/knee2/ankle2/ltoe2\" \"AL_usdmaya_ProxyShape1\"", results, false, true);
  EXPECT_EQ(1, results.length());
  EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|ltoe2"), results[0]);
  results.clear();

  EXPECT_EQ(1, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2")});

  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -a -pp \"/root/hip2/knee2/ankle2/rtoe2\" \"AL_usdmaya_ProxyShape1\"", results, false, true);
  EXPECT_EQ(1, results.length());
  EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|rtoe2"), results[0]);
  results.clear();

  EXPECT_EQ(2, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  MGlobal::executeCommand("undo", false, true);

  EXPECT_EQ(1, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2")});

  MGlobal::executeCommand("undo", false, true);

  EXPECT_EQ(0, proxy->selectedPaths().size());
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(0, sl.length());

  MGlobal::executeCommand("redo", false, true);

  EXPECT_EQ(1, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2")});


  MGlobal::executeCommand("redo", false, true);

  EXPECT_EQ(2, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -d -pp \"/root/hip2/knee2/ankle2/ltoe2\" \"AL_usdmaya_ProxyShape1\"", results, false, true);
  EXPECT_EQ(0, results.length());
  results.clear();

  EXPECT_EQ(1, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -d -pp \"/root/hip2/knee2/ankle2/rtoe2\" \"AL_usdmaya_ProxyShape1\"", results, false, true);
  EXPECT_EQ(0, results.length());
  results.clear();

  EXPECT_EQ(0, proxy->selectedPaths().size());
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(0, sl.length());

  MGlobal::executeCommand("undo", false, true);

  EXPECT_EQ(1, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  MGlobal::executeCommand("undo", false, true);

  EXPECT_EQ(2, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  MGlobal::executeCommand("redo", false, true);

  EXPECT_EQ(1, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  MGlobal::executeCommand("redo", false, true);

  EXPECT_EQ(0, proxy->selectedPaths().size());
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(0, sl.length());

  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -tgl -pp \"/root/hip2/knee2/ankle2/rtoe2\" -pp \"/root/hip2/knee2/ankle2/ltoe2\" \"AL_usdmaya_ProxyShape1\"", results, false, true);
  EXPECT_EQ(2, results.length());
  EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|rtoe2"), results[0]);
  EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|ltoe2"), results[1]);
  results.clear();

  EXPECT_EQ(2, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -tgl -pp \"/root/hip2/knee2/ankle2/rtoe2\" -pp \"/root/hip2/knee2/ankle2/ltoe2\" \"AL_usdmaya_ProxyShape1\"", results, false, true);
  EXPECT_EQ(0, results.length());
  results.clear();

  EXPECT_EQ(0, proxy->selectedPaths().size());
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(0, sl.length());

  MGlobal::executeCommand("undo", false, true);

  EXPECT_EQ(2, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  MGlobal::executeCommand("undo", false, true);

  EXPECT_EQ(0, proxy->selectedPaths().size());
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(0, sl.length());

  MGlobal::executeCommand("redo", false, true);

  EXPECT_EQ(2, proxy->selectedPaths().size());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  compareNodes({SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2")});

  MGlobal::executeCommand("redo", false, true);

  EXPECT_EQ(0, proxy->selectedPaths().size());
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(0, sl.length());
}

// make sure we can select a parent transform of a node that is already selected
TEST(ProxyShapeSelect, selectParent)
{
  MFileIO::newFile(true);
  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    UsdGeomXform knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
    UsdGeomXform ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
    UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
    return stage;
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_selectParent.usda";
  std::string sessionLayerContents;

  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MFnDagNode fn;
  MObject xform = fn.create("transform");
  MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);
  MString shapeName = fn.name();

  AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

  // force the stage to load
  proxy->filePathPlug().setString(temp_path.c_str());

  // select a single path
  MGlobal::executeCommand("select -cl;");
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" \"AL_usdmaya_ProxyShape1\"", false, true);
  MSelectionList sl;
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(1, sl.length());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1\" \"AL_usdmaya_ProxyShape1\"", false, true);
  sl.clear();
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(1, sl.length());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
}

// make sure we can select a parent transform of a node that is already selected (via the maya select command)
TEST(ProxyShapeSelect, selectParentViaMaya)
{
  MFileIO::newFile(true);
  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    UsdGeomXform knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
    UsdGeomXform ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
    UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
    return stage;
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_selectParent.usda";
  std::string sessionLayerContents;

  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MFnDagNode fn;
  MObject xform = fn.create("transform");
  MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);
  MString shapeName = fn.name();

  AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

  // force the stage to load
  proxy->filePathPlug().setString(temp_path.c_str());

  // select a single path
  MGlobal::executeCommand("select -cl;");
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" \"AL_usdmaya_ProxyShape1\"", false, true);
  MSelectionList sl;
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(1, sl.length());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

  MGlobal::executeCommand("select -r \"ankle1\"", false, true);
  sl.clear();
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(1, sl.length());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_FALSE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
}

// make sure we can select a parent transform of a node that is already selected (via the maya select command)
TEST(ProxyShapeSelect, selectSamePathTwice)
{
  MFileIO::newFile(true);
  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    UsdGeomXform knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
    UsdGeomXform ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
    UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
    return stage;
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_selectParent.usda";
  std::string sessionLayerContents;

  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MFnDagNode fn;
  MObject xform = fn.create("transform");
  MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);
  MString shapeName = fn.name();

  AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

  // force the stage to load
  proxy->filePathPlug().setString(temp_path.c_str());

  // select a single path
  MGlobal::executeCommand("select -cl;");
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" \"AL_usdmaya_ProxyShape1\"", false, true);
  MSelectionList sl;
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(1, sl.length());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

  uint32_t selected = 0, required = 0, refCount = 0;
  proxy->getCounts(SdfPath("/root/hip1/knee1/ankle1/ltoe1"), selected, required, refCount);
  EXPECT_EQ(1, selected);
  EXPECT_EQ(0, required);
  EXPECT_EQ(0, refCount);

  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" \"AL_usdmaya_ProxyShape1\"", false, true);
  sl.clear();
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(1, sl.length());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

  selected = 0, required = 0, refCount = 0;
  proxy->getCounts(SdfPath("/root/hip1/knee1/ankle1/ltoe1"), selected, required, refCount);
  EXPECT_EQ(1, selected);
  EXPECT_EQ(0, required);
  EXPECT_EQ(0, refCount);
}


// make sure we can select a parent transform of a node that is already selected (via the maya select command)
TEST(ProxyShapeSelect, selectSamePathTwiceViaMaya)
{
  MFileIO::newFile(true);
  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    UsdGeomXform knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
    UsdGeomXform ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
    UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
    return stage;
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_selectParent.usda";
  std::string sessionLayerContents;

  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MFnDagNode fn;
  MObject xform = fn.create("transform");
  MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);
  MString shapeName = fn.name();

  AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

  // force the stage to load
  proxy->filePathPlug().setString(temp_path.c_str());

  // select a single path
  MGlobal::executeCommand("select -cl;");
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" \"AL_usdmaya_ProxyShape1\"", false, true);
  MSelectionList sl;
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(1, sl.length());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

  uint32_t selected = 0, required = 0, refCount = 0;
  proxy->getCounts(SdfPath("/root/hip1/knee1/ankle1/ltoe1"), selected, required, refCount);
  EXPECT_EQ(1, selected);
  EXPECT_EQ(0, required);
  EXPECT_EQ(0, refCount);

  MGlobal::executeCommand("select -r \"|transform1|root|hip1|knee1|ankle1|ltoe1\"", false, true);
  sl.clear();
  MGlobal::getActiveSelectionList(sl);
  EXPECT_EQ(1, sl.length());
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1")));
  EXPECT_TRUE(proxy->isRequiredPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

  selected = 0, required = 0, refCount = 0;
  proxy->getCounts(SdfPath("/root/hip1/knee1/ankle1/ltoe1"), selected, required, refCount);
  EXPECT_EQ(1, selected);
  EXPECT_EQ(0, required);
  EXPECT_EQ(0, refCount);
}

TEST(ProxyShapeSelect, repeatedSelection)
{
  MFileIO::newFile(true);
  // unsure undo is enabled for this test
  MGlobal::executeCommand("undoInfo -state 1;");

  auto constructTransformChain = []() {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    return stage;
  };

  auto assertSelected = [](MString objName, const SdfPath& path, AL::usdmaya::nodes::ProxyShape *proxy) {
    MSelectionList sl;
    MGlobal::getActiveSelectionList(sl);
    MStringArray selStrings;
    sl.getSelectionStrings(selStrings);
    ASSERT_EQ(1, selStrings.length());
    ASSERT_EQ(objName, selStrings[0]);

    // Make sure it's only selected ONCE!
    auto& selectedPaths = proxy->selectedPaths();
    ASSERT_EQ(1, selectedPaths.size());
    size_t pathCount = 0;
    for (auto& it : proxy->selectedPaths()) {
      if (it == path) {
        pathCount += 1;
      }
    }
    ASSERT_EQ(1, pathCount);
  };

  auto assertNothingSelected = [](AL::usdmaya::nodes::ProxyShape *proxy) {
    MSelectionList sl;
    MGlobal::getActiveSelectionList(sl);
    ASSERT_EQ(0, sl.length());
    ASSERT_EQ(0, proxy->selectedPaths().size());
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_repeatedSelection.usda";

  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MFnDagNode fn;
  MObject xform = fn.create("transform");
  MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);
  MString shapeName = fn.name();

  SdfPath hipPath("/root/hip1");
  AL::usdmaya::nodes::ProxyShape *proxy = (AL::usdmaya::nodes::ProxyShape *) fn.userNode();

  // force the stage to load
  proxy->filePathPlug().setString(temp_path.c_str());
  MStringArray results;

  // select a single path
  MGlobal::executeCommand("select -cl;");
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -a -pp \"/root/hip1\" -pp \"/root/hip1\" -pp \"/root/hip1\" \"AL_usdmaya_ProxyShape1\"", results,
                          false, true);
  // Make sure it's selected
  { SCOPED_TRACE(""); assertSelected("hip1", hipPath, proxy); }

  // select it again
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -a -pp \"/root/hip1\" -pp \"/root/hip1\" -pp \"/root/hip1\" \"AL_usdmaya_ProxyShape1\"", results,
                          false, true);
  // Make sure it's still selected
  { SCOPED_TRACE(""); assertSelected("hip1", hipPath, proxy); }

  // deselect it
  MGlobal::executeCommand("AL_usdmaya_ProxyShapeSelect -d -pp \"/root/hip1\" \"AL_usdmaya_ProxyShape1\"", results,
                          false, true);
  { SCOPED_TRACE(""); assertNothingSelected(proxy); }

  // make sure undo /redo work as expected
  MGlobal::executeCommand("undo", false, true);
  { SCOPED_TRACE(""); assertSelected("hip1", hipPath, proxy); }
  MGlobal::executeCommand("redo", false, true);
  { SCOPED_TRACE(""); assertNothingSelected(proxy); }
  MGlobal::executeCommand("undo", false, true);
  { SCOPED_TRACE(""); assertSelected("hip1", hipPath, proxy); }
  MGlobal::executeCommand("undo", false, true);
  { SCOPED_TRACE(""); assertSelected("hip1", hipPath, proxy); }
  MGlobal::executeCommand("undo", false, true);
  { SCOPED_TRACE(""); assertNothingSelected(proxy); }
  MGlobal::executeCommand("redo", false, true);
  { SCOPED_TRACE(""); assertSelected("hip1", hipPath, proxy); }
  MGlobal::executeCommand("redo", false, true);
  { SCOPED_TRACE(""); assertSelected("hip1", hipPath, proxy); }
  MGlobal::executeCommand("redo", false, true);
  { SCOPED_TRACE(""); assertNothingSelected(proxy); }
  MGlobal::executeCommand("undo", false, true);
  { SCOPED_TRACE(""); assertSelected("hip1", hipPath, proxy); }
  MGlobal::executeCommand("undo", false, true);
  { SCOPED_TRACE(""); assertSelected("hip1", hipPath, proxy); }
  MGlobal::executeCommand("undo", false, true);
  { SCOPED_TRACE(""); assertNothingSelected(proxy); }
}
