//
// Copyright 2026 Autodesk
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

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsdUI/ui/mayaRenderSetupHost.h>

#ifdef MAYA_HAS_USD_SETTINGS_NODES
#include <mayaUsd/nodes/sceneRenderDescription.h>
#endif

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <ufe/pathString.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AdskUsdRenderSetup {
void PrintTo(const ExternalCamera& camera, std::ostream* os)
{
    *os << "{ \"" << camera.identifier << "\", \"" << camera.displayName << "\", \""
        << camera.groupLabel << "\" }";
}
} // namespace AdskUsdRenderSetup

namespace {

UsdStageRefPtr createProxyShapeStage(const std::string& name = {})
{
    MString shapeName;
    MGlobal::executeCommand(
        name.empty() ? MString("createNode mayaUsdProxyShape")
                     : MString("createNode mayaUsdProxyShape -name ") + name.c_str(),
        shapeName);
    MGlobal::executeCommand("connectAttr time1.outTime " + shapeName + ".time");

    MStringArray longNames;
    MGlobal::executeCommand("ls -long " + shapeName, longNames);
    if (longNames.length() != 1) {
        return {};
    }
    const UsdStageRefPtr stage
        = MayaUsd::ufe::getStage(Ufe::PathString::path(longNames[0].asChar()));

    // Adding a proxy shape rebuilds the UFE stage map, and that rebuild can
    // miss the proxy shapes created before it. A lookup by path adds them back.
    MStringArray allShapes;
    MGlobal::executeCommand("ls -long -type mayaUsdProxyShape", allShapes);
    for (const MString& shape : allShapes) {
        MayaUsd::ufe::getStage(Ufe::PathString::path(shape.asChar()));
    }
    return stage;
}

std::string undoName()
{
    MString name;
    MGlobal::executeCommand("undoInfo -q -undoName", name);
    return name.asChar();
}

// The undo queue is off while a command runs with undoEnabled false, the default.
void undo() { MGlobal::executeCommand("undo", false, true); }
void redo() { MGlobal::executeCommand("redo", false, true); }
void flushUndo() { MGlobal::executeCommand("flushUndo"); }

#ifdef MAYA_HAS_USD_SETTINGS_NODES
std::string stagePathString(const UsdStageRefPtr& stage)
{
    return Ufe::PathString::string(MayaUsd::ufe::stagePath(stage));
}
#endif

using ExternalCameras = std::vector<AdskUsdRenderSetup::ExternalCamera>;

ExternalCameras groupedCameras(const ExternalCameras& cameras)
{
    ExternalCameras grouped;
    std::copy_if(
        cameras.begin(),
        cameras.end(),
        std::back_inserter(grouped),
        [](const AdskUsdRenderSetup::ExternalCamera& camera) {
            return !camera.groupLabel.empty();
        });
    return grouped;
}

bool hasCamera(const ExternalCameras& cameras, const std::string& identifier)
{
    return std::any_of(
        cameras.begin(), cameras.end(), [&](const AdskUsdRenderSetup::ExternalCamera& camera) {
            return camera.identifier == identifier;
        });
}

const TfToken kCameraType("Camera");
const TfToken kCubeType("Cube");
const TfToken kScopeType("Scope");
const TfToken kXformType("Xform");

} // namespace

class MayaRenderSetupHostTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MGlobal::executeCommand("file -force -new");
        MGlobal::executeCommand("undoInfo -state on -infinity on", false, true);
        flushUndo();
    }

    MayaUsdRenderSetup::MayaRenderSetupHost host;
};

TEST_F(MayaRenderSetupHostTest, RenameIsOneUndoStepNamedAfterThePrim)
{
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    const UsdPrim cube = stage->DefinePrim(SdfPath("/Cube"), kCubeType);
    flushUndo();

    const SdfPath renamed = host.renamePrim(cube, "Box");
    ASSERT_EQ(renamed, SdfPath("/Box"));
    EXPECT_EQ(undoName(), "Rename_Cube");

    undo();
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/Cube")));
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath("/Box")));

    redo();
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath("/Cube")));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/Box")));
}

TEST_F(MayaRenderSetupHostTest, DuplicateOnSameStageIsOneUndoStep)
{
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    const UsdPrim cube = stage->DefinePrim(SdfPath("/Cube"), kCubeType);
    flushUndo();

    const SdfPath copy = host.duplicatePrim(cube, stage);
    ASSERT_FALSE(copy.IsEmpty());
    EXPECT_NE(copy, cube.GetPath());
    EXPECT_TRUE(stage->GetPrimAtPath(copy));
    EXPECT_EQ(undoName(), "Duplicate_Cube");

    undo();
    EXPECT_FALSE(stage->GetPrimAtPath(copy));
    EXPECT_TRUE(cube.IsValid());

    redo();
    EXPECT_TRUE(stage->GetPrimAtPath(copy));
}

TEST_F(MayaRenderSetupHostTest, DuplicateToOtherStageDefinesParentsAsScopesInOneUndoStep)
{
    const UsdStageRefPtr srcStage = createProxyShapeStage();
    const UsdStageRefPtr dstStage = createProxyShapeStage();
    ASSERT_TRUE(srcStage);
    ASSERT_TRUE(dstStage);
    srcStage->DefinePrim(SdfPath("/A"), kXformType);
    srcStage->DefinePrim(SdfPath("/A/B"), kXformType);
    const UsdPrim cube = srcStage->DefinePrim(SdfPath("/A/B/Cube"), kCubeType);
    flushUndo();

    const SdfPath copy = host.duplicatePrim(cube, dstStage);
    ASSERT_FALSE(copy.IsEmpty());
    EXPECT_EQ(copy.GetParentPath(), SdfPath("/A/B"));
    EXPECT_TRUE(dstStage->GetPrimAtPath(copy));
    for (const char* parent : { "/A", "/A/B" }) {
        const UsdPrim prim = dstStage->GetPrimAtPath(SdfPath(parent));
        ASSERT_TRUE(prim) << parent;
        EXPECT_TRUE(prim.IsDefined()) << parent;
        EXPECT_EQ(prim.GetTypeName(), kScopeType) << parent;
    }
    EXPECT_EQ(undoName(), "Duplicate_Cube");

    undo();
    EXPECT_FALSE(dstStage->GetPrimAtPath(copy));
    EXPECT_FALSE(dstStage->GetPrimAtPath(SdfPath("/A")));
    EXPECT_TRUE(cube.IsValid());

    redo();
    EXPECT_TRUE(dstStage->GetPrimAtPath(copy));
    EXPECT_TRUE(dstStage->GetPrimAtPath(SdfPath("/A/B")).IsDefined());
}

TEST_F(MayaRenderSetupHostTest, DuplicateWithStageOutsideMayaSceneDoesNothing)
{
    const UsdStageRefPtr sceneStage = createProxyShapeStage();
    ASSERT_TRUE(sceneStage);
    const UsdStageRefPtr orphanStage = UsdStage::CreateInMemory();
    const UsdPrim        sceneCube = sceneStage->DefinePrim(SdfPath("/Cube"), kCubeType);
    const UsdPrim        orphanCube = orphanStage->DefinePrim(SdfPath("/Orphan"), kCubeType);
    flushUndo();

    EXPECT_TRUE(host.duplicatePrim(sceneCube, orphanStage).IsEmpty());
    EXPECT_FALSE(orphanStage->GetPrimAtPath(SdfPath("/Cube")));

    EXPECT_TRUE(host.duplicatePrim(orphanCube, sceneStage).IsEmpty());
    EXPECT_FALSE(sceneStage->GetPrimAtPath(SdfPath("/Orphan")));

    EXPECT_TRUE(undoName().empty());
}

TEST_F(MayaRenderSetupHostTest, DuplicateRejectsInvalidPrimOrMissingTargetStage)
{
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    const UsdPrim cube = stage->DefinePrim(SdfPath("/Cube"), kCubeType);
    flushUndo();

    EXPECT_TRUE(host.duplicatePrim(UsdPrim(), stage).IsEmpty());
    EXPECT_TRUE(host.duplicatePrim(cube, UsdStageRefPtr()).IsEmpty());
    EXPECT_TRUE(undoName().empty());
}

TEST_F(MayaRenderSetupHostTest, DeleteIsUndoable)
{
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    stage->DefinePrim(SdfPath("/Cube"), kCubeType);
    flushUndo();

    EXPECT_TRUE(host.deletePrim(stage->GetPrimAtPath(SdfPath("/Cube"))));
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath("/Cube")));

    undo();
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/Cube")));

    redo();
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath("/Cube")));
}

TEST_F(MayaRenderSetupHostTest, DeleteInvalidPrimFails)
{
    EXPECT_FALSE(host.deletePrim(UsdPrim()));
}

TEST_F(MayaRenderSetupHostTest, DeleteOnLockedLayerIsRefusedAndKeepsThePrim)
{
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    const UsdPrim cube = stage->DefinePrim(SdfPath("/Cube"), kCubeType);
    stage->GetRootLayer()->SetPermissionToEdit(false);
    flushUndo();

    EXPECT_FALSE(host.deletePrim(cube));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/Cube")));
}

TEST_F(MayaRenderSetupHostTest, RenameToSiblingNameIsUniquified)
{
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    const UsdPrim cube = stage->DefinePrim(SdfPath("/Cube"), kCubeType);
    stage->DefinePrim(SdfPath("/Box"), kCubeType);
    flushUndo();

    const SdfPath renamed = host.renamePrim(cube, "Box");
    ASSERT_FALSE(renamed.IsEmpty());
    EXPECT_NE(renamed, SdfPath("/Box"));
    EXPECT_EQ(renamed.GetParentPath(), SdfPath::AbsoluteRootPath());
    EXPECT_TRUE(stage->GetPrimAtPath(renamed));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/Box")));
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath("/Cube")));
}

TEST_F(MayaRenderSetupHostTest, RenameRejectsInvalidPrimOrEmptyName)
{
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    const UsdPrim cube = stage->DefinePrim(SdfPath("/Cube"), kCubeType);
    flushUndo();

    EXPECT_TRUE(host.renamePrim(UsdPrim(), "Box").IsEmpty());
    EXPECT_TRUE(host.renamePrim(cube, "").IsEmpty());
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/Cube")));
    EXPECT_TRUE(undoName().empty());
}

TEST_F(MayaRenderSetupHostTest, RenameOnLockedLayerFailsAndKeepsThePrim)
{
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    const UsdPrim cube = stage->DefinePrim(SdfPath("/Cube"), kCubeType);
    stage->GetRootLayer()->SetPermissionToEdit(false);
    flushUndo();

    EXPECT_TRUE(host.renamePrim(cube, "Box").IsEmpty());
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/Cube")));
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath("/Box")));
}

// A stage with no proxy shape, like the Maya Settings stage, has no UFE path,
// so the edits go through the base implementation inside a Maya undo block.
TEST_F(MayaRenderSetupHostTest, RenameOnStageWithoutProxyShapeIsOneUndoStep)
{
    const UsdStageRefPtr stage = UsdStage::CreateInMemory();
    const UsdPrim        cube = stage->DefinePrim(SdfPath("/Cube"), kCubeType);
    flushUndo();

    EXPECT_EQ(host.renamePrim(cube, "Box"), SdfPath("/Box"));
    EXPECT_EQ(undoName(), "Rename_Cube");

    undo();
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/Cube")));
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath("/Box")));

    redo();
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath("/Cube")));
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/Box")));
}

TEST_F(MayaRenderSetupHostTest, DeleteOnStageWithoutProxyShapeIsOneUndoStep)
{
    const UsdStageRefPtr stage = UsdStage::CreateInMemory();
    const UsdPrim        cube = stage->DefinePrim(SdfPath("/Cube"), kCubeType);
    flushUndo();

    EXPECT_TRUE(host.deletePrim(cube));
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath("/Cube")));
    EXPECT_EQ(undoName(), "Delete_Cube");

    undo();
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/Cube")));

    redo();
    EXPECT_FALSE(stage->GetPrimAtPath(SdfPath("/Cube")));
}

TEST_F(MayaRenderSetupHostTest, CurrentFrameAndTimelineRangeFollowMaya)
{
    MGlobal::executeCommand("playbackOptions -min 5 -max 42");
    MGlobal::executeCommand("currentTime 12");

    EXPECT_DOUBLE_EQ(host.currentFrame(), 12.0);
    const AdskUsdRenderSetup::FrameRange range = host.timelineRange();
    EXPECT_DOUBLE_EQ(range.start, 5.0);
    EXPECT_DOUBLE_EQ(range.end, 42.0);
}

TEST_F(MayaRenderSetupHostTest, HostNameIsMaya) { EXPECT_EQ(host.hostName(), "Maya"); }

TEST_F(MayaRenderSetupHostTest, PrettifyNameSplitsCamelCase)
{
    EXPECT_EQ(host.prettifyName("renderingColorSpace"), "Rendering Color Space");
}

TEST_F(MayaRenderSetupHostTest, ExternalCamerasListMayaCamerasByTransform)
{
    const ExternalCameras cameras = host.externalCameras(UsdStageRefPtr());

    EXPECT_TRUE(hasCamera(cameras, "|persp"));
    for (const auto& camera : cameras) {
        EXPECT_EQ(camera.identifier, "|" + camera.displayName);
        EXPECT_TRUE(camera.groupLabel.empty()) << camera.identifier;
    }
}

TEST_F(MayaRenderSetupHostTest, ExternalCamerasSkipIntermediateCameraShapes)
{
    MStringArray created;
    MGlobal::executeCommand("camera", created);
    ASSERT_EQ(created.length(), 2u);
    const std::string identifier = std::string("|") + created[0].asChar();
    EXPECT_TRUE(hasCamera(host.externalCameras(UsdStageRefPtr()), identifier));

    MGlobal::executeCommand("setAttr " + created[1] + ".intermediateObject 1");
    EXPECT_FALSE(hasCamera(host.externalCameras(UsdStageRefPtr()), identifier));
}

TEST_F(MayaRenderSetupHostTest, ExternalCamerasForProxyShapeStageListNoUsdCameras)
{
    const UsdStageRefPtr stage = createProxyShapeStage();
    const UsdStageRefPtr otherStage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    ASSERT_TRUE(otherStage);
    otherStage->DefinePrim(SdfPath("/Cam"), kCameraType);

    EXPECT_TRUE(groupedCameras(host.externalCameras(stage)).empty());
}

#ifdef MAYA_HAS_USD_SETTINGS_NODES

TEST_F(MayaRenderSetupHostTest, ExternalCamerasForRenderDescriptionStageGroupsProxyShapeCameras)
{
    const UsdStageRefPtr settingsStage = MayaUsd::SceneRenderDescription::getUsdStage();
    ASSERT_TRUE(settingsStage);
    const UsdStageRefPtr zStage = createProxyShapeStage("zStage");
    const UsdStageRefPtr aStage = createProxyShapeStage("aStage");
    ASSERT_TRUE(zStage);
    ASSERT_TRUE(aStage);
    zStage->DefinePrim(SdfPath("/ZCam"), kCameraType);
    aStage->DefinePrim(SdfPath("/ACam"), kCameraType);
    aStage->DefinePrim(SdfPath("/Cube"), kCubeType);

    const ExternalCameras expected {
        { stagePathString(aStage) + ",/ACam", "/ACam", "aStage" },
        { stagePathString(zStage) + ",/ZCam", "/ZCam", "zStage" },
    };
    EXPECT_EQ(groupedCameras(host.externalCameras(settingsStage)), expected);
}

TEST_F(MayaRenderSetupHostTest, ExternalCamerasFollowCameraPrimEdits)
{
    const UsdStageRefPtr settingsStage = MayaUsd::SceneRenderDescription::getUsdStage();
    ASSERT_TRUE(settingsStage);
    const UsdStageRefPtr stage = createProxyShapeStage("stage");
    ASSERT_TRUE(stage);
    stage->DefinePrim(SdfPath("/CamA"), kCameraType);
    ASSERT_EQ(groupedCameras(host.externalCameras(settingsStage)).size(), 1u);

    stage->DefinePrim(SdfPath("/CamB"), kCameraType);
    EXPECT_EQ(groupedCameras(host.externalCameras(settingsStage)).size(), 2u);

    stage->RemovePrim(SdfPath("/CamA"));
    const ExternalCameras remaining = groupedCameras(host.externalCameras(settingsStage));
    ASSERT_EQ(remaining.size(), 1u);
    EXPECT_EQ(remaining[0].displayName, "/CamB");
}

TEST_F(MayaRenderSetupHostTest, ActiveRenderDescriptionRoundTripsThroughTheSettingsNode)
{
    ASSERT_TRUE(MayaUsd::SceneRenderDescription::getUsdStage());
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);

    const AdskUsdRenderSetup::RenderDescription description { stage, SdfPath("/Render/Settings") };
    host.setActiveRenderDescription(description);
    EXPECT_EQ(
        MayaUsd::SceneRenderDescription::getActiveRenderDescriptionPath(),
        stagePathString(stage) + ",/Render/Settings");
    EXPECT_EQ(host.activeRenderDescription(), description);

    host.setActiveRenderDescription({});
    EXPECT_TRUE(MayaUsd::SceneRenderDescription::getActiveRenderDescriptionPath().empty());
    EXPECT_TRUE(host.activeRenderDescription().isEmpty());
}

TEST_F(MayaRenderSetupHostTest, SetActiveRenderDescriptionIgnoresStageOutsideMayaScene)
{
    ASSERT_TRUE(MayaUsd::SceneRenderDescription::getUsdStage());
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    const AdskUsdRenderSetup::RenderDescription description { stage, SdfPath("/Render/Settings") };
    host.setActiveRenderDescription(description);

    host.setActiveRenderDescription({ UsdStage::CreateInMemory(), SdfPath("/Render/Settings") });
    EXPECT_EQ(host.activeRenderDescription(), description);
}

TEST_F(MayaRenderSetupHostTest, UnresolvableActiveRenderDescriptionPathReadsAsEmpty)
{
    ASSERT_TRUE(MayaUsd::SceneRenderDescription::getUsdStage());

    for (const char* storedPath : { "|persp", "|noSuchNode|noSuchShape,/Render/Settings" }) {
        MayaUsd::SceneRenderDescription::setActiveRenderDescriptionPath(storedPath);
        EXPECT_TRUE(host.activeRenderDescription().isEmpty()) << storedPath;
    }
}

#endif

TEST_F(MayaRenderSetupHostTest, RenderingColorSpacePreferenceFollowsColorManagement)
{
    int wasEnabled = 0;
    MGlobal::executeCommand("colorManagementPrefs -q -cmEnabled", wasEnabled);

    MGlobal::executeCommand("colorManagementPrefs -e -cmEnabled 0");
    EXPECT_TRUE(host.renderingColorSpacePreference().empty());

    MGlobal::executeCommand("colorManagementPrefs -e -cmEnabled 1");
    const MString renderingSpace
        = MGlobal::executeCommandStringResult("colorManagementPrefs -q -renderingSpaceName");
    ASSERT_NE(renderingSpace.length(), 0u);
    EXPECT_EQ(host.renderingColorSpacePreference(), renderingSpace.asChar());

    MGlobal::executeCommand(
        ("colorManagementPrefs -e -cmEnabled " + std::to_string(wasEnabled)).c_str());
}

TEST_F(MayaRenderSetupHostTest, RenderingColorSpacesIncludeTheCurrentRenderingSpace)
{
    const std::string renderingSpace
        = MGlobal::executeCommandStringResult("colorManagementPrefs -q -renderingSpaceName")
              .asChar();

    const std::vector<std::string> spaces = host.renderingColorSpaces();
    EXPECT_NE(std::find(spaces.begin(), spaces.end(), renderingSpace), spaces.end());
}
