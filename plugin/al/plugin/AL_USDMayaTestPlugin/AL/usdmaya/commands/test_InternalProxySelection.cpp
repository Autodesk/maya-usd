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
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "test_usdmaya.h"

#include <pxr/usd/usdGeom/xform.h>

#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>

using AL::maya::test::buildTempPath;

TEST(InternalProxyShapeSelect, selectNode)
{
    MFileIO::newFile(true);
    // unsure undo is enabled for this test
    MGlobal::executeCommand("undoInfo -state 1;");

    auto constructTransformChain = []() {
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

    const std::string temp_path = buildTempPath("AL_USDMayaTests_internalSelectNode.usda");
    std::string       sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
    MString    shapeName = fn.name();

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());

    // select a single path
    MGlobal::executeCommand(
        "AL_usdmaya_InternalProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    MSelectionList sl;

    // make sure the path is contained in the selected paths (for hydra selection)
    EXPECT_EQ(1u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

    // make sure undo clears the previous info
    MGlobal::executeCommand("undo", false, true);
    EXPECT_EQ(0u, proxy->selectionList().size());

    // make sure redo works happily without side effects
    MGlobal::executeCommand("redo", false, true);
    EXPECT_EQ(1u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

    // So now we have a single item selected. Let's see if we can replace this selection list
    // with two other paths. The previous selection should be removed, the two additional paths
    // should be selected
    MGlobal::executeCommand(
        "AL_usdmaya_InternalProxyShapeSelect -r -pp \"/root/hip2/knee2/ankle2/ltoe2\" -pp "
        "\"/root/hip2/knee2/ankle2/rtoe2\" \"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    EXPECT_EQ(2u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    // when undoing this command, the previous path should be selected
    MGlobal::executeCommand("undo", false, true);
    EXPECT_EQ(1u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

    MGlobal::executeCommand("redo", false, true);
    EXPECT_EQ(2u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    // now attempt to clear the selection list
    MGlobal::executeCommand(
        "AL_usdmaya_InternalProxyShapeSelect -cl \"AL_usdmaya_ProxyShape1\"", false, true);
    EXPECT_EQ(0u, proxy->selectionList().size());

    // undoing this command should return selected items back into
    MGlobal::executeCommand("undo", false, true);
    EXPECT_EQ(2u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    MGlobal::executeCommand("redo", false, true);
    EXPECT_EQ(0u, proxy->selectionList().size());

    // So now we have a single item selected. Let's see if we can replace this selection list
    // with two other paths. The previous selection should be removed, the two additional paths
    // should be selected
    MGlobal::executeCommand(
        "AL_usdmaya_InternalProxyShapeSelect -a -pp \"/root/hip2/knee2/ankle2/ltoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    EXPECT_EQ(1u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));

    MGlobal::executeCommand(
        "AL_usdmaya_InternalProxyShapeSelect -a -pp \"/root/hip2/knee2/ankle2/rtoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    EXPECT_EQ(2u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    MGlobal::executeCommand("undo", false, true);
    EXPECT_EQ(1u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));

    MGlobal::executeCommand("undo", false, true);
    EXPECT_EQ(0u, proxy->selectionList().size());

    MGlobal::executeCommand("redo", false, true);
    EXPECT_EQ(1u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));

    MGlobal::executeCommand("redo", false, true);
    EXPECT_EQ(2u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    MGlobal::executeCommand(
        "AL_usdmaya_InternalProxyShapeSelect -d -pp \"/root/hip2/knee2/ankle2/ltoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    EXPECT_EQ(1u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    MGlobal::executeCommand(
        "AL_usdmaya_InternalProxyShapeSelect -d -pp \"/root/hip2/knee2/ankle2/rtoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    EXPECT_EQ(0u, proxy->selectionList().size());

    MGlobal::executeCommand("undo", false, true);
    EXPECT_EQ(1u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    MGlobal::executeCommand("undo", false, true);
    EXPECT_EQ(2u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    MGlobal::executeCommand("redo", false, true);
    EXPECT_EQ(1u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    MGlobal::executeCommand("redo", false, true);
    EXPECT_EQ(0u, proxy->selectionList().size());

    MGlobal::executeCommand(
        "AL_usdmaya_InternalProxyShapeSelect -tgl -pp \"/root/hip2/knee2/ankle2/rtoe2\" -pp "
        "\"/root/hip2/knee2/ankle2/ltoe2\" \"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    EXPECT_EQ(2u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    MGlobal::executeCommand(
        "AL_usdmaya_InternalProxyShapeSelect -tgl -pp \"/root/hip2/knee2/ankle2/rtoe2\" -pp "
        "\"/root/hip2/knee2/ankle2/ltoe2\" \"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    EXPECT_EQ(0u, proxy->selectionList().size());

    MGlobal::executeCommand("undo", false, true);
    EXPECT_EQ(2u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    MGlobal::executeCommand("undo", false, true);
    EXPECT_EQ(0u, proxy->selectionList().size());

    MGlobal::executeCommand("redo", false, true);
    EXPECT_EQ(2u, proxy->selectionList().size());
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(proxy->selectionList().isSelected(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    MGlobal::executeCommand("redo", false, true);
    EXPECT_EQ(0u, proxy->selectionList().size());
}
