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

#include <mayaUsd/nodes/proxyShapePlugin.h>

#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>
#include <maya/MStringArray.h>

#if defined(WANT_UFE_BUILD)
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/pathString.h>
#endif
#endif

using AL::maya::test::buildTempPath;

namespace {

bool isAlive(AL::usdmaya::nodes::ProxyShape* proxy, SdfPath path)
{
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        return proxy->isRequiredPath(path);
    }

    // Look for the specified path or a descendent of it in selection
    Ufe::PathSegment ps_usd(path.GetText(), USD_UFE_RUNTIME_ID, USD_UFE_SEPARATOR);
    Ufe::Path        ufePath(proxy->ufePath() + ps_usd);
    auto             selection = Ufe::GlobalSelection::get();
    return selection->contains(ufePath) || selection->containsDescendant(ufePath);
}

bool isSelected(AL::usdmaya::nodes::ProxyShape* proxy, SdfPath path)
{
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        return proxy->isRequiredPath(path);
    }

    auto             proxyShapePath = proxy->ufePath();
    Ufe::PathSegment ps_usd(path.GetText(), USD_UFE_RUNTIME_ID, USD_UFE_SEPARATOR);
    auto             selection = Ufe::GlobalSelection::get();
    return selection->contains(proxyShapePath + ps_usd);
}

bool containsSdfPath(const Ufe::Path& ufePath, const SdfPathVector& paths)
{
    // Check if the speficied UFE path has one of the supplied SdfPaths
    auto it = find_if(paths.begin(), paths.end(), [&ufePath](const SdfPath& sdfPath) {
        auto segments = ufePath.getSegments();
        return segments.size() > 1 && segments[1].string() == sdfPath.GetText();
    });
    return it != paths.end();
}
} // namespace

TEST(ProxyShapeSelect, selectNode1)
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

    auto compareNodes = [](const SdfPathVector& paths) {
        if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
            MSelectionList sl;
            MGlobal::getActiveSelectionList(sl);
            EXPECT_EQ(sl.length(), paths.size());

            for (uint32_t i = 0; i < sl.length(); ++i) {
                MObject obj;
                sl.getDependNode(i, obj);
                MFnDependencyNode fn(obj);
                MStatus           status;
                MPlug             plug = fn.findPlug("primPath", &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);
                MString pathName = plug.asString();
                bool    found = false;
                for (uint32_t j = 0; j < paths.size(); ++j) {
                    if (pathName == paths[j].GetText()) {
                        found = true;
                        break;
                    }
                }
                EXPECT_TRUE(found);
            }
        } else {
            auto selection = Ufe::GlobalSelection::get();
            for (auto it : *selection) {
                EXPECT_TRUE(containsSdfPath(it->path(), paths));
            }
        }
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_selectNode.usda");
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
    MGlobal::executeCommand("select -cl;");
    MStringArray results;
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    MSelectionList sl;

    EXPECT_EQ(1u, results.length());
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(MString("|transform1|root|hip1|knee1|ankle1|ltoe1"), results[0]);
    } else {
        #ifdef UFE_V2_FEATURES_AVAILABLE
            EXPECT_EQ(
                Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1/root/hip1/knee1/ankle1/ltoe1"),
                Ufe::PathString::path(results[0].asChar()));
        #else
            EXPECT_EQ(
                MString("|world|transform1|AL_usdmaya_ProxyShape1/root/hip1/knee1/ankle1/ltoe1"),
                results[0]);
        #endif
    }
    results.clear();

    // make sure the path is contained in the selected paths (for hydra selection)
    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes({ SdfPath("/root/hip1/knee1/ankle1/ltoe1") });

    // make sure undo clears the previous info
    MGlobal::executeCommand("undo", false, true);
    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());

    // make sure redo works happily without side effects
    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes({ SdfPath("/root/hip1/knee1/ankle1/ltoe1") });

    // So now we have a single item selected. Let's see if we can replace this selection list
    // with two other paths. The previous selection should be removed, the two additional paths
    // should be selected
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip2/knee2/ankle2/ltoe2\" -pp "
        "\"/root/hip2/knee2/ankle2/rtoe2\" \"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    EXPECT_EQ(2u, results.length());
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|ltoe2"), results[0]);
        EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|rtoe2"), results[1]);
    } else {
        #ifdef UFE_V2_FEATURES_AVAILABLE
            EXPECT_EQ(
                Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/ltoe2"),
                Ufe::PathString::path(results[0].asChar()));
            EXPECT_EQ(
                Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/rtoe2"),
                Ufe::PathString::path(results[1].asChar()));
        #else
            EXPECT_EQ(
                MString("|world|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/ltoe2"),
                results[0]);
            EXPECT_EQ(
                MString("|world|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/rtoe2"),
                results[1]);
        #endif
    }
    results.clear();

    EXPECT_EQ(2u, proxy->selectedPaths().size());
    EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));

    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes(
        { SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    // when undoing this command, the previous path should be selected
    MGlobal::executeCommand("undo", false, true);

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    compareNodes({ SdfPath("/root/hip1/knee1/ankle1/ltoe1") });

    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(2u, proxy->selectedPaths().size());
    EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes(
        { SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    // now attempt to clear the selection list
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -cl \"AL_usdmaya_ProxyShape1\"", results, false, true);
    EXPECT_EQ(0u, results.length());

    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());

    // undoing this command should return selected items back into
    MGlobal::executeCommand("undo", false, true);

    EXPECT_EQ(2u, proxy->selectedPaths().size());
    EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes(
        { SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());
}

TEST(ProxyShapeSelect, selectNode2)
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

    auto compareNodes = [](const SdfPathVector& paths) {
        if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
            MSelectionList sl;
            MGlobal::getActiveSelectionList(sl);
            EXPECT_EQ(sl.length(), paths.size());

            for (uint32_t i = 0; i < sl.length(); ++i) {
                MObject obj;
                sl.getDependNode(i, obj);
                MFnDependencyNode fn(obj);
                MStatus           status;
                MPlug             plug = fn.findPlug("primPath", &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);
                MString pathName = plug.asString();
                bool    found = false;
                for (uint32_t j = 0; j < paths.size(); ++j) {
                    if (pathName == paths[j].GetText()) {
                        found = true;
                        break;
                    }
                }
                EXPECT_TRUE(found);
            }
        } else {
            auto selection = Ufe::GlobalSelection::get();
            for (auto it : *selection) {
                EXPECT_TRUE(containsSdfPath(it->path(), paths));
            }
        }
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_selectNode.usda");
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
    MGlobal::executeCommand("select -cl;");
    MStringArray   results;
    MSelectionList sl;

    // So now we have a single item selected. Let's see if we can replace this selection list
    // with two other paths. The previous selection should be removed, the two additional paths
    // should be selected
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -a -pp \"/root/hip2/knee2/ankle2/ltoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    EXPECT_EQ(1u, results.length());
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|ltoe2"), results[0]);
    } else {
        #ifdef UFE_V2_FEATURES_AVAILABLE
            EXPECT_EQ(
                Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/ltoe2"),
                Ufe::PathString::path(results[0].asChar()));
        #else
            EXPECT_EQ(
                MString("|world|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/ltoe2"),
                results[0]);
        #endif
    }
    results.clear();

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    compareNodes({ SdfPath("/root/hip2/knee2/ankle2/ltoe2") });

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -a -pp \"/root/hip2/knee2/ankle2/rtoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    EXPECT_EQ(1u, results.length());
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|rtoe2"), results[0]);
    } else {
        #ifdef UFE_V2_FEATURES_AVAILABLE
            EXPECT_EQ(
                Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/rtoe2"),
                Ufe::PathString::path(results[0].asChar()));
        #else
            EXPECT_EQ(
                MString("|world|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/rtoe2"),
                results[0]);
        #endif
    }
    results.clear();

    EXPECT_EQ(2u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes(
        { SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand("undo", false, true);

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes({ SdfPath("/root/hip2/knee2/ankle2/ltoe2") });

    MGlobal::executeCommand("undo", false, true);

    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());

    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes({ SdfPath("/root/hip2/knee2/ankle2/ltoe2") });

    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(2u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes(
        { SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -d -pp \"/root/hip2/knee2/ankle2/ltoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    EXPECT_EQ(0u, results.length());
    results.clear();

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes({ SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -d -pp \"/root/hip2/knee2/ankle2/rtoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    EXPECT_EQ(0u, results.length());
    results.clear();

    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());

    MGlobal::executeCommand("undo", false, true);

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes({ SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand("undo", false, true);

    EXPECT_EQ(2u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes(
        { SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes({ SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());
}

TEST(ProxyShapeSelect, selectNode3)
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

    auto compareNodes = [](const SdfPathVector& paths) {
        if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
            MSelectionList sl;
            MGlobal::getActiveSelectionList(sl);
            EXPECT_EQ(sl.length(), paths.size());

            for (uint32_t i = 0; i < sl.length(); ++i) {
                MObject obj;
                sl.getDependNode(i, obj);
                MFnDependencyNode fn(obj);
                MStatus           status;
                MPlug             plug = fn.findPlug("primPath", &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);
                MString pathName = plug.asString();
                bool    found = false;
                for (uint32_t j = 0; j < paths.size(); ++j) {
                    if (pathName == paths[j].GetText()) {
                        found = true;
                        break;
                    }
                }
                EXPECT_TRUE(found);
            }
        } else {
            auto selection = Ufe::GlobalSelection::get();
            for (auto it : *selection) {
                EXPECT_TRUE(containsSdfPath(it->path(), paths));
            }
        }
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_selectNode.usda");
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
    MGlobal::executeCommand("select -cl;");
    MStringArray   results;
    MSelectionList sl;

    // So now we have a single item selected. Let's see if we can replace this selection list
    // with two other paths. The previous selection should be removed, the two additional paths
    // should be selected
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -a -pp \"/root/hip2/knee2/ankle2/ltoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -a -pp \"/root/hip2/knee2/ankle2/rtoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -d -pp \"/root/hip2/knee2/ankle2/ltoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    EXPECT_EQ(0u, results.length());
    results.clear();

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes({ SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -d -pp \"/root/hip2/knee2/ankle2/rtoe2\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    EXPECT_EQ(0u, results.length());
    results.clear();

    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());

    MGlobal::executeCommand("undo", false, true);

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes({ SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand("undo", false, true);

    EXPECT_EQ(2u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes(
        { SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(1u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes({ SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -tgl -pp \"/root/hip2/knee2/ankle2/rtoe2\" -pp "
        "\"/root/hip2/knee2/ankle2/ltoe2\" \"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    EXPECT_EQ(2u, results.length());
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|rtoe2"), results[0]);
        EXPECT_EQ(MString("|transform1|root|hip2|knee2|ankle2|ltoe2"), results[1]);
    } else {
        #ifdef UFE_V2_FEATURES_AVAILABLE
            EXPECT_EQ(
                Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/rtoe2"),
                Ufe::PathString::path(results[0].asChar()));
            EXPECT_EQ(
                Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/ltoe2"),
                Ufe::PathString::path(results[1].asChar()));
        #else
            EXPECT_EQ(
                MString("|world|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/rtoe2"),
                results[0]);
            EXPECT_EQ(
                MString("|world|transform1|AL_usdmaya_ProxyShape1/root/hip2/knee2/ankle2/ltoe2"),
                results[1]);
        #endif
    }
    results.clear();

    EXPECT_EQ(2u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes(
        { SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -tgl -pp \"/root/hip2/knee2/ankle2/rtoe2\" -pp "
        "\"/root/hip2/knee2/ankle2/ltoe2\" \"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    EXPECT_EQ(0u, results.length());
    results.clear();

    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());

    MGlobal::executeCommand("undo", false, true);

    EXPECT_EQ(2u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes(
        { SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand("undo", false, true);

    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());

    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(2u, proxy->selectedPaths().size());
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    compareNodes(
        { SdfPath("/root/hip2/knee2/ankle2/ltoe2"), SdfPath("/root/hip2/knee2/ankle2/rtoe2") });

    MGlobal::executeCommand("redo", false, true);

    EXPECT_EQ(0u, proxy->selectedPaths().size());
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip2/knee2/ankle2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/ltoe2")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip2/knee2/ankle2/rtoe2")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_FALSE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
    MGlobal::getActiveSelectionList(sl);
    EXPECT_EQ(0u, sl.length());
}

// make sure we can select a parent transform of a node that is already selected
TEST(ProxyShapeSelect, selectParent)
{
    MFileIO::newFile(true);
    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomXform   leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
        UsdGeomXform   knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
        UsdGeomXform   ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
        UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
        return stage;
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_selectParent.usda");
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
    MGlobal::executeCommand("select -cl;");
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);
        EXPECT_EQ(1u, sl.length());
    } else {
        auto selection = Ufe::GlobalSelection::get();
        EXPECT_EQ(1u, selection->size());
    }
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1\" \"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);
        EXPECT_EQ(1u, sl.length());
    } else {
        auto selection = Ufe::GlobalSelection::get();
        EXPECT_EQ(1u, selection->size());
    }
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
}

// make sure we can select a parent transform of a node that is already selected (via the maya
// select command)
TEST(ProxyShapeSelect, selectParentViaMaya)
{
    MFileIO::newFile(true);
    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomXform   leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
        UsdGeomXform   knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
        UsdGeomXform   ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
        UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
        return stage;
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_selectParent.usda");
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
    MGlobal::executeCommand("select -cl;");
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);
        EXPECT_EQ(1u, sl.length());
    } else {
        auto selection = Ufe::GlobalSelection::get();
        EXPECT_EQ(1u, selection->size());
    }
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1\" \"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);
        EXPECT_EQ(1u, sl.length());
    } else {
        auto selection = Ufe::GlobalSelection::get();
        EXPECT_EQ(1u, selection->size());
    }
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_FALSE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
}

// make sure we can select a parent transform of a node that is already selected (via the maya
// select command)
TEST(ProxyShapeSelect, selectSamePathTwice)
{
    MFileIO::newFile(true);
    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomXform   leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
        UsdGeomXform   knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
        UsdGeomXform   ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
        UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
        return stage;
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_selectParent.usda");
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
    MGlobal::executeCommand("select -cl;");
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);
        EXPECT_EQ(1u, sl.length());
    } else {
        auto selection = Ufe::GlobalSelection::get();
        EXPECT_EQ(1u, selection->size());
    }
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

    uint32_t selected = 0, required = 0, refCount = 0;
    proxy->getCounts(SdfPath("/root/hip1/knee1/ankle1/ltoe1"), selected, required, refCount);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(1u, selected);
    } else {
        EXPECT_EQ(0u, selected);
    }
    EXPECT_EQ(0u, required);
    EXPECT_EQ(0u, refCount);

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);
        EXPECT_EQ(1u, sl.length());
    } else {
        auto selection = Ufe::GlobalSelection::get();
        EXPECT_EQ(1u, selection->size());
    }
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

    selected = 0, required = 0, refCount = 0;
    proxy->getCounts(SdfPath("/root/hip1/knee1/ankle1/ltoe1"), selected, required, refCount);
    // no ref counts will be generated for selected UFE transforms
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(1u, selected);
    }
    EXPECT_EQ(0u, required);
    EXPECT_EQ(0u, refCount);
}

// make sure we can select a parent transform of a node that is already selected (via the maya
// select command)
TEST(ProxyShapeSelect, selectSamePathTwiceViaMaya)
{
    MFileIO::newFile(true);
    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomXform   leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
        UsdGeomXform   knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
        UsdGeomXform   ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
        UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
        return stage;
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_selectParent.usda");
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
    MGlobal::executeCommand("select -cl;");
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1/knee1/ankle1/ltoe1\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        true);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);
        EXPECT_EQ(1u, sl.length());
    } else {
        auto selection = Ufe::GlobalSelection::get();
        EXPECT_EQ(1u, selection->size());
    }
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

    uint32_t selected = 0, required = 0, refCount = 0;
    proxy->getCounts(SdfPath("/root/hip1/knee1/ankle1/ltoe1"), selected, required, refCount);
    // no ref counts will be generated for selected UFE transforms
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(1u, selected);
    }
    EXPECT_EQ(0u, required);
    EXPECT_EQ(0u, refCount);

    MGlobal::executeCommand("select -r \"|transform1|root|hip1|knee1|ankle1|ltoe1\"", false, true);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);
        EXPECT_EQ(1u, sl.length());
    } else {
        auto selection = Ufe::GlobalSelection::get();
        EXPECT_EQ(1u, selection->size());
    }
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
    EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
    EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));

    selected = 0, required = 0, refCount = 0;
    proxy->getCounts(SdfPath("/root/hip1/knee1/ankle1/ltoe1"), selected, required, refCount);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(1u, selected);
    }
    EXPECT_EQ(0u, required);
    EXPECT_EQ(0u, refCount);
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

    auto assertSelected
        = [](MString objName, const SdfPath& path, AL::usdmaya::nodes::ProxyShape* proxy) {
              if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
                  MSelectionList sl;
                  MGlobal::getActiveSelectionList(sl);
                  MStringArray selStrings;
                  sl.getSelectionStrings(selStrings);
                  ASSERT_EQ(1u, selStrings.length());
                  ASSERT_EQ(objName, selStrings[0]);

                  // Make sure it's only selected ONCE!
                  auto& selectedPaths = proxy->selectedPaths();
                  ASSERT_EQ(1u, selectedPaths.size());
                  size_t pathCount = 0;
                  for (auto& it : proxy->selectedPaths()) {
                      if (it == path) {
                          pathCount += 1;
                      }
                  }
                  ASSERT_EQ(1u, pathCount);
              } else {
                  auto   selection = Ufe::GlobalSelection::get();
                  size_t pathCount = 0;
                  for (auto it : *selection) {
                      auto segments = it->path().getSegments();
                      if (segments.size() > 1 && segments[1].string() == path.GetText()) {
                          ++pathCount;
                      }
                  }
                  EXPECT_EQ(1u, pathCount);
              }
          };

    auto assertNothingSelected = [](AL::usdmaya::nodes::ProxyShape* proxy) {
        if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
            MSelectionList sl;
            MGlobal::getActiveSelectionList(sl);
            ASSERT_EQ(0u, sl.length());
            ASSERT_EQ(0u, proxy->selectedPaths().size());
        } else {
            auto selection = Ufe::GlobalSelection::get();
            EXPECT_EQ(0u, selection->size());
        }
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_repeatedSelection.usda");

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
    MString    shapeName = fn.name();

    SdfPath                         hipPath("/root/hip1");
    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());
    MStringArray results;

    // select a single path
    MGlobal::executeCommand("select -cl;");
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -a -pp \"/root/hip1\" -pp \"/root/hip1\" -pp \"/root/hip1\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    // Make sure it's selected
    {
        SCOPED_TRACE("");
        assertSelected("hip1", hipPath, proxy);
    }

    // select it again
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -a -pp \"/root/hip1\" -pp \"/root/hip1\" -pp \"/root/hip1\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    // Make sure it's still selected
    {
        SCOPED_TRACE("");
        assertSelected("hip1", hipPath, proxy);
    }

    // deselect it
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -d -pp \"/root/hip1\" \"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    {
        SCOPED_TRACE("");
        assertNothingSelected(proxy);
    }

    // make sure undo /redo work as expected
    MGlobal::executeCommand("undo", false, true);
    {
        SCOPED_TRACE("");
        assertSelected("hip1", hipPath, proxy);
    }
    MGlobal::executeCommand("redo", false, true);
    {
        SCOPED_TRACE("");
        assertNothingSelected(proxy);
    }
    MGlobal::executeCommand("undo", false, true);
    {
        SCOPED_TRACE("");
        assertSelected("hip1", hipPath, proxy);
    }
    MGlobal::executeCommand("undo", false, true);
    {
        SCOPED_TRACE("");
        assertSelected("hip1", hipPath, proxy);
    }
    MGlobal::executeCommand("undo", false, true);
    {
        SCOPED_TRACE("");
        assertNothingSelected(proxy);
    }
    MGlobal::executeCommand("redo", false, true);
    {
        SCOPED_TRACE("");
        assertSelected("hip1", hipPath, proxy);
    }
    MGlobal::executeCommand("redo", false, true);
    {
        SCOPED_TRACE("");
        assertSelected("hip1", hipPath, proxy);
    }
    MGlobal::executeCommand("redo", false, true);
    {
        SCOPED_TRACE("");
        assertNothingSelected(proxy);
    }
    MGlobal::executeCommand("undo", false, true);
    {
        SCOPED_TRACE("");
        assertSelected("hip1", hipPath, proxy);
    }
    MGlobal::executeCommand("undo", false, true);
    {
        SCOPED_TRACE("");
        assertSelected("hip1", hipPath, proxy);
    }
    MGlobal::executeCommand("undo", false, true);
    {
        SCOPED_TRACE("");
        assertNothingSelected(proxy);
    }
}

TEST(ProxyShapeSelect, deselectNode)
{
    MFileIO::newFile(true);
    // unsure undo is enabled for this test
    MGlobal::executeCommand("undoInfo -state 1;");

    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();

        UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
        UsdGeomCamera::Define(stage, SdfPath("/root/cam"));
        return stage;
    };

    auto assertSelected = [](MString objName) {
        if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
            MSelectionList sl;
            MGlobal::getActiveSelectionList(sl);
            MStringArray selStrings;
            sl.getSelectionStrings(selStrings);
            ASSERT_EQ(1u, selStrings.length());
            ASSERT_EQ(objName, selStrings[0]);
        } else {
            auto selection = Ufe::GlobalSelection::get();
            ASSERT_EQ(1u, selection->size());
            auto str = (*selection->begin())->path().string();
            EXPECT_TRUE(str.find(objName.asChar()) != std::string::npos);
        }
    };

    auto assertNothingSelected = []() {
        if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
            MSelectionList sl;
            MGlobal::getActiveSelectionList(sl);
            ASSERT_EQ(0u, sl.length());
        } else {
            auto selection = Ufe::GlobalSelection::get();
            ASSERT_EQ(0u, selection->size());
        }
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_deselectNode.usda");
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
    MStringArray results;

    // select a single path
    MGlobal::executeCommand("select -cl;");
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/hip1\" \"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    // Make sure it's selected
    assertSelected("hip1");

    // deselect it
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -d -pp \"/root/hip1\" \"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    // Make sure it's deselected
    assertNothingSelected();

    // make sure undo /redo work as expected
    MGlobal::executeCommand("undo", false, true);
    assertSelected("hip1");
    MGlobal::executeCommand("redo", false, true);
    assertNothingSelected();
    MGlobal::executeCommand("undo", false, true);
    assertSelected("hip1");
    MGlobal::executeCommand("undo", false, true);
    assertNothingSelected();

    // Make sure that all the above works, even with an object that will not be destroyed when
    // deselected (ie, a cam) select a single path
    MGlobal::executeCommand("select -cl;");
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/root/cam\" \"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    // Make sure it's selected
    assertSelected("cam");

    // deselect it
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -d -pp \"/root/cam\" \"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    // Make sure it's deselected
    assertNothingSelected();

    // make sure undo /redo work as expected
    MGlobal::executeCommand("undo", false, true);
    assertSelected("cam");
    MGlobal::executeCommand("redo", false, true);
    assertNothingSelected();
    MGlobal::executeCommand("undo", false, true);
    assertSelected("cam");
    MGlobal::executeCommand("undo", false, true);
    assertNothingSelected();
}

TEST(ProxyShapeSelect, pseudoRootSelect)
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

    auto compareNodes = [](const SdfPathVector& paths) {
        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);
        EXPECT_EQ(sl.length(), paths.size());
        for (uint32_t i = 0; i < sl.length(); ++i) {
            MObject obj;
            sl.getDependNode(i, obj);
            MFnDependencyNode fn(obj);
            MString           pathName;
            if (fn.typeId() == AL::usdmaya::nodes::ProxyShape::kTypeId) {
                EXPECT_EQ(
                    MString("|transform1|AL_usdmaya_ProxyShape1"), MFnDagNode(obj).fullPathName());
                pathName = "/";
            } else {
                MStatus status;
                MPlug   plug = fn.findPlug("primPath", &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);
                pathName = plug.asString();
            }
            bool found = false;
            for (uint32_t j = 0; j < paths.size(); ++j) {
                if (pathName == paths[j].GetText()) {
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found);
        }
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_selectNode.usda");
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

    MStringArray results;

    // select the root of the usd stage. No transforms should be generated, but the proxy shape
    // itself should be selected.
    MGlobal::executeCommand("select -cl;");

    // State 0: nothing selected

    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -pp \"/\" \"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    EXPECT_EQ(1u, results.length());
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(MString("|transform1|AL_usdmaya_ProxyShape1"), results[0]);
    } else {
        #ifdef UFE_V2_FEATURES_AVAILABLE
            EXPECT_EQ(
                Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1"),
                Ufe::PathString::path(results[0].asChar()));
        #else
            EXPECT_EQ(MString("|world|transform1|AL_usdmaya_ProxyShape1"), results[0]);
        #endif
    }
    results.clear();

    // State 1: proxy selected
    // make sure nothing is in the internally selected paths
    EXPECT_EQ(0u, proxy->selectedPaths().size());
    // Make sure active selection is as expected
    {
        SCOPED_TRACE("");
        compareNodes({ SdfPath("/") });
    };

    // make sure undo clears the previous info
    MGlobal::executeCommand("undo", false, true);

    // State 0: nothing selected
    EXPECT_EQ(0u, proxy->selectedPaths().size());
    {
        SCOPED_TRACE("");
        compareNodes({});
    };

    // make sure redo works happily without side effects
    MGlobal::executeCommand("redo", false, true);

    // State 1: proxy selected
    EXPECT_EQ(0u, proxy->selectedPaths().size());
    {
        SCOPED_TRACE("");
        compareNodes({ SdfPath("/") });
    };

    // Make sure toggle works with a root path, and another path
    MGlobal::executeCommand(
        "AL_usdmaya_ProxyShapeSelect -r -tgl -pp \"/root/hip1/knee1/ankle1/ltoe1\" -pp \"/\" "
        "\"AL_usdmaya_ProxyShape1\"",
        results,
        false,
        true);
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        EXPECT_EQ(MString("|transform1|root|hip1|knee1|ankle1|ltoe1"), results[0]);
    } else {
        #ifdef UFE_V2_FEATURES_AVAILABLE
            EXPECT_EQ(
                Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1/root/hip1/knee1/ankle1/ltoe1"),
                Ufe::PathString::path(results[0].asChar()));
        #else
            EXPECT_EQ(
                MString("|world|transform1|AL_usdmaya_ProxyShape1/root/hip1/knee1/ankle1/ltoe1"),
                results[0]);
        #endif
    }
    results.clear();

    // RB: Disabling this test for now as this is currently broken in UFE pending some feedback.
    // on how to select the root.
    //
    //
    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        // State 2: ltoe1 selected
        // make sure ltoe1 is contained in the selected paths (for hydra selection)
        EXPECT_EQ(1u, proxy->selectedPaths().size());
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        // Ensure the proxyShape is no longer selected, and the other sub-path is
        {
            SCOPED_TRACE("");
            compareNodes({ SdfPath("/root/hip1/knee1/ankle1/ltoe1") });
        };

        // Make sure append works
        MGlobal::executeCommand(
            "AL_usdmaya_ProxyShapeSelect -append -pp \"/\" -pp \"/root/hip2\" "
            "\"AL_usdmaya_ProxyShape1\"",
            results,
            false,
            true);
        EXPECT_EQ(2u, results.length());
        if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
            EXPECT_EQ(MString("|transform1|AL_usdmaya_ProxyShape1"), results[0]);
            EXPECT_EQ(MString("|transform1|root|hip2"), results[1]);
        } else {
            #ifdef UFE_V2_FEATURES_AVAILABLE
                EXPECT_EQ(
                    Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1"),
                    Ufe::PathString::path(results[0].asChar()));
                EXPECT_EQ(
                    Ufe::PathString::path("|transform1|AL_usdmaya_ProxyShape1/root/hip2"),
                    Ufe::PathString::path(results[1].asChar()));
            #else
                EXPECT_EQ(MString("|world|transform1|AL_usdmaya_ProxyShape1"), results[0]);
                EXPECT_EQ(MString("|world|transform1|AL_usdmaya_ProxyShape1/root/hip2"), results[1]);
            #endif
        }
        results.clear();

        // State 3: ltoe1, proxy, hip2 selected
        // make sure ltoe1 and hip2 are contained in the selected paths (for hydra selection)
        EXPECT_EQ(2u, proxy->selectedPaths().size());
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2")));
        // Ensure the proxyShape is selected again, as well as ltoe2 and hip2
        {
            SCOPED_TRACE("");
            compareNodes(
                { SdfPath("/root/hip1/knee1/ankle1/ltoe1"), SdfPath("/"), SdfPath("/root/hip2") });
        };

        // Make sure remove works
        MGlobal::executeCommand(
            "AL_usdmaya_ProxyShapeSelect -d -pp \"/\" \"AL_usdmaya_ProxyShape1\"",
            results,
            false,
            true);
        EXPECT_EQ(0u, results.length());

        // State 4: ltoe1, hip2 selected
        // make sure ltoe1 and hip2 are contained in the selected paths (for hydra selection)
        EXPECT_EQ(2u, proxy->selectedPaths().size());
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2")));
        // Ensure the proxyShape is no longer selected
        {
            SCOPED_TRACE("");
            compareNodes({ SdfPath("/root/hip1/knee1/ankle1/ltoe1"), SdfPath("/root/hip2") });
        };

        // Undo the remove
        MGlobal::executeCommand("undo", false, true);

        // State 3: ltoe1, proxy, hip2 selected
        // make sure ltoe1 and hip2 are contained in the selected paths (for hydra selection)
        EXPECT_EQ(2u, proxy->selectedPaths().size());
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2")));
        // Ensure the proxyShape is selected again, as well as ltoe2 and hip2
        {
            SCOPED_TRACE("");
            compareNodes(
                { SdfPath("/root/hip1/knee1/ankle1/ltoe1"), SdfPath("/"), SdfPath("/root/hip2") });
        };

        // Undo the append
        MGlobal::executeCommand("undo", false, true);

        // State 2: ltoe1 selected
        // make sure ltoe1 is contained in the selected paths (for hydra selection)
        EXPECT_EQ(1u, proxy->selectedPaths().size());
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        // Ensure the proxyShape is no longer selected, and the other sub-path is
        {
            SCOPED_TRACE("");
            compareNodes({ SdfPath("/root/hip1/knee1/ankle1/ltoe1") });
        };

        // Undo the toggle
        MGlobal::executeCommand("undo", false, true);

        // State 1: proxy selected
        EXPECT_EQ(0u, proxy->selectedPaths().size());
        {
            SCOPED_TRACE("");
            compareNodes({ SdfPath("/") });
        };

        // Undo the initial replace
        MGlobal::executeCommand("undo", false, true);

        // State 0: nothing selected
        EXPECT_EQ(0u, proxy->selectedPaths().size());
        {
            SCOPED_TRACE("");
            compareNodes({});
        };

        // Redo the initial replace
        MGlobal::executeCommand("redo", false, true);

        // State 1: proxy selected
        EXPECT_EQ(0u, proxy->selectedPaths().size());
        {
            SCOPED_TRACE("");
            compareNodes({ SdfPath("/") });
        };

        // Redo the toggle
        MGlobal::executeCommand("redo", false, true);

        // State 2: ltoe1 selected
        // make sure ltoe1 is contained in the selected paths (for hydra selection)
        EXPECT_EQ(1u, proxy->selectedPaths().size());
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        // Ensure the proxyShape is no longer selected, and the other sub-path is
        {
            SCOPED_TRACE("");
            compareNodes({ SdfPath("/root/hip1/knee1/ankle1/ltoe1") });
        };

        // Redo the append
        MGlobal::executeCommand("redo", false, true);

        // State 3: ltoe1, proxy, hip2 selected
        // make sure ltoe1 and hip2 are contained in the selected paths (for hydra selection)
        EXPECT_EQ(2u, proxy->selectedPaths().size());
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2")));
        // Ensure the proxyShape is selected again, as well as ltoe2 and hip2
        {
            SCOPED_TRACE("");
            compareNodes(
                { SdfPath("/root/hip1/knee1/ankle1/ltoe1"), SdfPath("/"), SdfPath("/root/hip2") });
        };

        // Redo the remove
        MGlobal::executeCommand("redo", false, true);

        // State 4: ltoe1, hip2 selected
        // make sure ltoe1 and hip2 are contained in the selected paths (for hydra selection)
        EXPECT_EQ(2u, proxy->selectedPaths().size());
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_EQ(1u, proxy->selectedPaths().count(SdfPath("/root/hip2")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1")));
        EXPECT_TRUE(isAlive(proxy, SdfPath("/root/hip1/knee1/ankle1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip1/knee1/ankle1/ltoe1")));
        EXPECT_TRUE(isSelected(proxy, SdfPath("/root/hip2")));
        // Ensure the proxyShape is no longer selected
        {
            SCOPED_TRACE("");
            compareNodes({ SdfPath("/root/hip1/knee1/ankle1/ltoe1"), SdfPath("/root/hip2") });
        };
    }
}