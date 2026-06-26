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
#include "scopedLayerEditorDCCFunctions.h"

#include <layerEditorDCCFunctions.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdUtils/stageCache.h>

#include <gtest/gtest.h>

#include <QtWidgets/QWidget>

#include <cstdint>

using namespace UsdLayerEditor;

// mainWindowParent() returns null when unset, the registered widget when set.
TEST(LayerEditorDCCFunctions, MainWindowParent_DefaultsToNull)
{
    ScopedLayerEditorDCCFunctions guard;
    setEnvironmentFns(EnvironmentFns {});
    EXPECT_EQ(mainWindowParent(), nullptr);
}

TEST(LayerEditorDCCFunctions, MainWindowParent_ReturnsRegisteredWidget)
{
    ScopedLayerEditorDCCFunctions guard;
    QWidget        w;
    EnvironmentFns env;
    env.mainWindowParent = [&w]() { return &w; };
    setEnvironmentFns(env);
    EXPECT_EQ(mainWindowParent(), &w);
}

// layer-contents size limits default to 8, return the registered values when set.
TEST(LayerEditorDCCFunctions, LayerContentsLimits_DefaultToEight)
{
    ScopedLayerEditorDCCFunctions guard;
    setEnvironmentFns(EnvironmentFns {});
    EXPECT_EQ(layerContentsArraySizeLimit(), 8);
    EXPECT_EQ(layerContentsTimeSamplesSizeLimit(), 8);
}

TEST(LayerEditorDCCFunctions, LayerContentsLimits_ReturnRegisteredValues)
{
    ScopedLayerEditorDCCFunctions guard;
    EnvironmentFns                env;
    env.layerContentsArraySizeLimit = []() -> int64_t { return 3; };
    env.layerContentsTimeSamplesSizeLimit = []() -> int64_t { return 5; };
    setEnvironmentFns(env);
    EXPECT_EQ(layerContentsArraySizeLimit(), 3);
    EXPECT_EQ(layerContentsTimeSamplesSizeLimit(), 5);
}

// captureSessionLayer grabs a proxy's current session layer so its opinions can be
// transferred onto the recreated stage after a rename/repath; returns null when unset.
TEST(LayerEditorDCCFunctions, CaptureSessionLayer_NullByDefault)
{
    ScopedLayerEditorDCCFunctions guard;
    setSerializationFns(SerializationFns {});
    EXPECT_FALSE(captureSessionLayer("|x"));
}

TEST(LayerEditorDCCFunctions, TransferSessionLayer_DispatchesWhenRegistered)
{
    ScopedLayerEditorDCCFunctions guard;
    PXR_NS::SdfLayerRefPtr seenSrc;
    std::string            seenDst;
    SerializationFns       fns;
    fns.transferSessionLayer
        = [&](const PXR_NS::SdfLayerRefPtr& src, const std::string& dst) { seenSrc = src; seenDst = dst; };
    setSerializationFns(fns);
    auto layer = PXR_NS::SdfLayer::CreateAnonymous("xfer");
    transferSessionLayer(layer, "|newProxy");
    EXPECT_EQ(seenSrc, layer);
    EXPECT_EQ(seenDst, "|newProxy");
}

// ── FileSystemFns ──────────────────────────────────────────────────────────

TEST(LayerEditorDCCFunctions, GetDCCSceneDir_DefaultsToEmpty)
{
    ScopedLayerEditorDCCFunctions guard;
    setFileSystemFns(FileSystemFns{});
    EXPECT_EQ(getDCCSceneDir(), std::string{});
}

TEST(LayerEditorDCCFunctions, GetDCCSceneDir_ReturnsRegisteredValue)
{
    ScopedLayerEditorDCCFunctions guard;
    FileSystemFns fns;
    fns.getDCCSceneDir = []() { return std::string("/scene/dir"); };
    setFileSystemFns(fns);
    EXPECT_EQ(getDCCSceneDir(), "/scene/dir");
}

TEST(LayerEditorDCCFunctions, GetDCCWorkspaceScenesDir_DefaultsToEmpty)
{
    ScopedLayerEditorDCCFunctions guard;
    setFileSystemFns(FileSystemFns{});
    EXPECT_EQ(getDCCWorkspaceScenesDir(), std::string{});
}

TEST(LayerEditorDCCFunctions, GetDCCWorkspaceScenesDir_ReturnsRegisteredValue)
{
    ScopedLayerEditorDCCFunctions guard;
    FileSystemFns fns;
    fns.getDCCWorkspaceScenesDir = []() { return std::string("/workspace/scenes"); };
    setFileSystemFns(fns);
    EXPECT_EQ(getDCCWorkspaceScenesDir(), "/workspace/scenes");
}

TEST(LayerEditorDCCFunctions, PrepareLayerSaveUILayer_DefaultsToTrue)
{
    ScopedLayerEditorDCCFunctions guard;
    setFileSystemFns(FileSystemFns{});
    EXPECT_TRUE(prepareLayerSaveUILayer("/some/dir"));
}

TEST(LayerEditorDCCFunctions, PrepareLayerSaveUILayer_DispatchesWhenRegistered)
{
    ScopedLayerEditorDCCFunctions guard;
    std::string seenAnchor;
    FileSystemFns fns;
    fns.prepareLayerSaveUILayer = [&](const std::string& anchor) -> bool {
        seenAnchor = anchor;
        return false;
    };
    setFileSystemFns(fns);
    EXPECT_FALSE(prepareLayerSaveUILayer("/my/anchor"));
    EXPECT_EQ(seenAnchor, "/my/anchor");
}

TEST(LayerEditorDCCFunctions, CheckWriteAccess_DefaultsToFalse)
{
    ScopedLayerEditorDCCFunctions guard;
    setFileSystemFns(FileSystemFns{});
    EXPECT_FALSE(checkWriteAccess("/tmp/test.usd"));
}

TEST(LayerEditorDCCFunctions, CheckWriteAccess_DispatchesWhenRegistered)
{
    ScopedLayerEditorDCCFunctions guard;
    std::string seenPath;
    FileSystemFns fns;
    fns.checkWriteAccess = [&](const std::string& path) -> bool {
        seenPath = path;
        return true;
    };
    setFileSystemFns(fns);
    EXPECT_TRUE(checkWriteAccess("/tmp/layer.usd"));
    EXPECT_EQ(seenPath, "/tmp/layer.usd");
}

// ── SerializationFns ────────────────────────────────────────────────────────

TEST(LayerEditorDCCFunctions, GetStageCaches_DefaultsToUtilsStageCache)
{
    ScopedLayerEditorDCCFunctions guard;
    setSerializationFns(SerializationFns{});
    auto caches = getStageCaches();
    ASSERT_EQ(caches.size(), 1u);
    EXPECT_EQ(caches[0], &PXR_NS::UsdUtilsStageCache::Get());
}

TEST(LayerEditorDCCFunctions, GetStageCaches_ReturnsRegisteredCaches)
{
    ScopedLayerEditorDCCFunctions guard;
    PXR_NS::UsdStageCache          extra;
    SerializationFns fns;
    fns.getStageCaches = [&]() {
        return std::vector<PXR_NS::UsdStageCache*>{ &extra };
    };
    setSerializationFns(fns);
    auto caches = getStageCaches();
    ASSERT_EQ(caches.size(), 1u);
    EXPECT_EQ(caches[0], &extra);
}

TEST(LayerEditorDCCFunctions, GetAllStages_ReturnsRegisteredStages)
{
    ScopedLayerEditorDCCFunctions guard;
    auto             stage = PXR_NS::UsdStage::CreateInMemory();
    SerializationFns fns;
    fns.getAllStages = [&]() { return std::vector<PXR_NS::UsdStageRefPtr> { stage }; };
    setSerializationFns(fns);
    auto stages = getAllStages();
    ASSERT_EQ(stages.size(), 1u);
    EXPECT_EQ(stages[0], stage);
}

TEST(LayerEditorDCCFunctions, SetLayerUpAxisAndUnits_DispatchesWhenRegistered)
{
    ScopedLayerEditorDCCFunctions guard;
    PXR_NS::SdfLayerRefPtr seenLayer;
    SerializationFns fns;
    fns.setLayerUpAxisAndUnits = [&](const PXR_NS::SdfLayerRefPtr& l) { seenLayer = l; };
    setSerializationFns(fns);
    auto layer = PXR_NS::SdfLayer::CreateAnonymous("upaxis");
    setLayerUpAxisAndUnits(layer);
    EXPECT_EQ(seenLayer, layer);
}

TEST(LayerEditorDCCFunctions, UpdateDCCObjectRootLayer_DispatchesWhenRegistered)
{
    ScopedLayerEditorDCCFunctions guard;
    std::string seenProxy, seenPath;
    bool        seenTarget = false;
    DccObjectRootLayerPathMode seenMode = DccObjectRootLayerPathMode::ForceAbsolute;
    SerializationFns fns;
    fns.updateDCCObjectRootLayer
        = [&](const std::string& proxy,
              const std::string& path,
              const PXR_NS::SdfLayerRefPtr&,
              bool                       isTarget,
              DccObjectRootLayerPathMode mode) {
              seenProxy  = proxy;
              seenPath   = path;
              seenTarget = isTarget;
              seenMode   = mode;
          };
    setSerializationFns(fns);
    updateDCCObjectRootLayer("|proxy", "/tmp/new.usd", nullptr, true);
    EXPECT_EQ(seenProxy, "|proxy");
    EXPECT_EQ(seenPath, "/tmp/new.usd");
    EXPECT_TRUE(seenTarget);
    EXPECT_EQ(seenMode, DccObjectRootLayerPathMode::FollowPreference); // default when omitted

    updateDCCObjectRootLayer(
        "|proxy", "/tmp/new.usd", nullptr, false, DccObjectRootLayerPathMode::ForceAbsolute);
    EXPECT_EQ(seenMode, DccObjectRootLayerPathMode::ForceAbsolute);
}
