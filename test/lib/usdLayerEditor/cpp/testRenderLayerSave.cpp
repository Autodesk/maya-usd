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
// Render layers are owned by a registry on the stage's root layer rather than by
// composition, so the save flow has to ask the DCC about them. These tests stand in for
// the DCC with stub RenderLayerFns, so none of them needs the render setup component.
//
#include "layerEditorDCCFunctions.h"
#include "layerLocking.h"
#include "saveLayersDialog.h"
#include "scopedLayerEditorDCCFunctions.h"
#include "scopedTempDir.h"
#include "utilSerialization.h"

#include <pxr/base/vt/dictionary.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <QtWidgets/QLineEdit>
#include <ghc/fs_std.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include <testFixture.h>

namespace UsdLayerEditor {

namespace {

// Installs a getRenderLayersToSave that always reports the given layers.
void stubRenderLayers(const std::vector<PXR_NS::SdfLayerRefPtr>& renderLayers)
{
    RenderLayerFns fns;
    fns.getRenderLayersToSave
        = [renderLayers](const PXR_NS::UsdStageRefPtr&) { return renderLayers; };
    setRenderLayerFns(fns);
}

const Serialization::LayerInfo* findAnonLayer(
    const Serialization::StageLayersToSave& layersInfo,
    const PXR_NS::SdfLayerRefPtr&           layer)
{
    for (const auto& info : layersInfo._anonLayers) {
        if (info.layer == layer)
            return &info;
    }
    return nullptr;
}

size_t countAnonLayer(
    const Serialization::StageLayersToSave& layersInfo,
    const PXR_NS::SdfLayerRefPtr&           layer)
{
    return static_cast<size_t>(std::count_if(
        layersInfo._anonLayers.begin(),
        layersInfo._anonLayers.end(),
        [&layer](const Serialization::LayerInfo& info) { return info.layer == layer; }));
}

size_t countDirtyLayer(
    const Serialization::StageLayersToSave& layersInfo,
    const PXR_NS::SdfLayerRefPtr&           layer)
{
    return static_cast<size_t>(std::count(
        layersInfo._dirtyFileBackedLayers.begin(), layersInfo._dirtyFileBackedLayers.end(), layer));
}

#if PXR_VERSION >= 2408
// Composes the layer the way Render Setup does for its active render layer: through a sub-layer
// expression on the session layer, which a walk of the sub-layer paths cannot resolve.
void setActiveRenderLayerExpression(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            identifier)
{
    auto                 session = stage->GetSessionLayer();
    PXR_NS::VtDictionary variables;
    variables["adskActiveRenderLayer"] = PXR_NS::VtValue(identifier);
    session->SetExpressionVariables(variables);
    if (session->GetNumSubLayerPaths() == 0) {
        session->InsertSubLayerPath("`${adskActiveRenderLayer}`", 0);
    }
}
#endif

} // namespace

// --- getLayersToSaveFromStage -------------------------------------------------

TEST(RenderLayerSave, GetLayersToSaveFromStage_CollectsInactiveAnonymousRenderLayer)
{
    ScopedLayerEditorDCCFunctions guard;

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    // Not a sub-layer of anything: this is what an inactive render layer looks like to
    // the composition walk.
    auto renderLayer = PXR_NS::SdfLayer::CreateAnonymous("RenderLayer_1");
    stubRenderLayers({ renderLayer });

    Serialization::StageLayersToSave layersInfo;
    Serialization::getLayersToSaveFromStage(stage, "test_obj", layersInfo);

    // The render layer and the anonymous root layer.
    EXPECT_EQ(layersInfo._anonLayers.size(), 2u);
    const auto* info = findAnonLayer(layersInfo, renderLayer);
    ASSERT_NE(info, nullptr) << "an inactive render layer must still be collected";
    EXPECT_EQ(info->parent._layerParent, nullptr);
}

TEST(RenderLayerSave, GetLayersToSaveFromStage_SavesRenderLayersBeforeTheRootLayer)
{
    ScopedLayerEditorDCCFunctions guard;

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto renderLayer = PXR_NS::SdfLayer::CreateAnonymous("RenderLayer_1");
    stubRenderLayers({ renderLayer });

    Serialization::StageLayersToSave layersInfo;
    Serialization::getLayersToSaveFromStage(stage, "test_obj", layersInfo);

    // Anonymous layers are saved in this order, and the registry the render layer lives in
    // is metadata on the root layer. Saving the root first would drop the new identifier.
    ASSERT_FALSE(layersInfo._anonLayers.empty());
    EXPECT_EQ(layersInfo._anonLayers.front().layer, renderLayer);
    EXPECT_EQ(layersInfo._anonLayers.back().layer, stage->GetRootLayer());
}

TEST(RenderLayerSave, GetLayersToSaveFromStage_CollectsDirtyFileBackedRenderLayerOnce)
{
    ScopedLayerEditorDCCFunctions guard;
    ScopedTempDir                 tempDir("le_render_layer_save_test");

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto renderLayer = PXR_NS::SdfLayer::CreateNew(tempDir.file("renderLayer1.usda"));
    ASSERT_TRUE(renderLayer);
    renderLayer->SetComment("dirty");
    ASSERT_TRUE(renderLayer->IsDirty());

    stubRenderLayers({ renderLayer });

    Serialization::StageLayersToSave layersInfo;
    Serialization::getLayersToSaveFromStage(stage, "test_obj", layersInfo);

    EXPECT_EQ(countDirtyLayer(layersInfo, renderLayer), 1u);
    // Saved in place, so there is nothing to re-path and no anonymous row for it.
    EXPECT_EQ(countAnonLayer(layersInfo, renderLayer), 0u);
}

TEST(RenderLayerSave, GetLayersToSaveFromStage_WithoutRenderLayerFns_IsUnchanged)
{
    ScopedLayerEditorDCCFunctions guard;
    setRenderLayerFns(RenderLayerFns {});

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sublayer_plain");
    stage->GetRootLayer()->InsertSubLayerPath(sublayer->GetIdentifier(), 0);

    Serialization::StageLayersToSave layersInfo;
    Serialization::getLayersToSaveFromStage(stage, "test_obj", layersInfo);

    EXPECT_EQ(layersInfo._anonLayers.size(), 2u);
}

#if PXR_VERSION >= 2408
TEST(RenderLayerSave, GetLayersToSaveFromStage_CollectsActiveRenderLayerOnce)
{
    ScopedLayerEditorDCCFunctions guard;

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto renderLayer = PXR_NS::SdfLayer::CreateAnonymous("RenderLayer_1");
    setActiveRenderLayerExpression(stage, renderLayer->GetIdentifier());
    ASSERT_TRUE(stage->HasLocalLayer(renderLayer));
    stubRenderLayers({ renderLayer });

    Serialization::StageLayersToSave layersInfo;
    Serialization::getLayersToSaveFromStage(stage, "test_obj", layersInfo);

    EXPECT_EQ(countAnonLayer(layersInfo, renderLayer), 1u);
}

TEST(RenderLayerSave, GetLayersToSaveFromStage_CollectsActiveDirtyFileBackedRenderLayerOnce)
{
    ScopedLayerEditorDCCFunctions guard;
    ScopedTempDir                 tempDir("le_active_dirty_render_layer_test");

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto renderLayer = PXR_NS::SdfLayer::CreateNew(tempDir.file("renderLayer1.usda"));
    ASSERT_TRUE(renderLayer);
    setActiveRenderLayerExpression(stage, renderLayer->GetIdentifier());
    ASSERT_TRUE(stage->HasLocalLayer(renderLayer));
    renderLayer->SetComment("dirty");
    stubRenderLayers({ renderLayer });

    Serialization::StageLayersToSave layersInfo;
    Serialization::getLayersToSaveFromStage(stage, "test_obj", layersInfo);

    EXPECT_EQ(countDirtyLayer(layersInfo, renderLayer), 1u);
}
#endif

TEST(RenderLayerSave, GetSaveableRenderLayers_IncludesLayersInTheLayerStack)
{
    ScopedLayerEditorDCCFunctions guard;

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto inactive = PXR_NS::SdfLayer::CreateAnonymous("RenderLayer_1");
    auto active = PXR_NS::SdfLayer::CreateAnonymous("RenderLayer_2");
    stage->GetRootLayer()->GetSubLayerPaths().push_back(active->GetIdentifier());
    stubRenderLayers({ inactive, active });

    const auto layers = Serialization::getSaveableRenderLayers(stage);

    ASSERT_EQ(layers.size(), 2u);
    EXPECT_EQ(layers[0], inactive);
    EXPECT_EQ(layers[1], active);
}

TEST(RenderLayerSave, SaveDirtyRenderLayers_SavesFileBackedAndLeavesAnonymousToTheDialog)
{
    ScopedLayerEditorDCCFunctions guard;
    ScopedTempDir                 tempDir("le_save_dirty_render_layers_test");

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto fileLayer = PXR_NS::SdfLayer::CreateNew(tempDir.file("renderLayer1.usd"));
    fileLayer->SetComment("edited");
    auto anonLayer = PXR_NS::SdfLayer::CreateAnonymous("RenderLayer_2");
    anonLayer->SetComment("edited");
    ASSERT_TRUE(fileLayer->IsDirty());
    stubRenderLayers({ fileLayer, anonLayer });

    const auto failed = Serialization::saveDirtyRenderLayers(stage);

    EXPECT_TRUE(failed.empty());
    EXPECT_FALSE(fileLayer->IsDirty());
    EXPECT_TRUE(anonLayer->IsDirty()) << "an anonymous layer needs a file name from the dialog";
}

TEST(RenderLayerSave, SaveDirtyRenderLayers_ReportsLayersThatFailToSave)
{
    ScopedLayerEditorDCCFunctions guard;
    ScopedTempDir                 tempDir("le_save_failing_render_layer_test");

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto layer = PXR_NS::SdfLayer::CreateNew(tempDir.file("failing.usd"));
    layer->SetComment("edited");
    stubRenderLayers({ layer });

    // A directory where the file belongs cannot be written.
    fs::filesystem::remove(tempDir.file("failing.usd"));
    fs::filesystem::create_directory(tempDir.file("failing.usd"));

    const auto failed = Serialization::saveDirtyRenderLayers(stage);

    ASSERT_EQ(failed.size(), 1u);
    EXPECT_EQ(failed[0], layer);
}

TEST(RenderLayerSave, GetSaveableRenderLayers_ExcludesSystemLockedLayer)
{
    ScopedLayerEditorDCCFunctions guard;
    struct ForgetSystemLocks
    {
        ~ForgetSystemLocks() { forgetSystemLockedLayers(); }
    } forgetLocks;

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto layer = PXR_NS::SdfLayer::CreateAnonymous("RenderLayer_1");
    lockLayer("", layer, LayerLock_SystemLocked, /*updateDCCObjectAttr=*/false);
    stubRenderLayers({ layer });

    EXPECT_TRUE(Serialization::getSaveableRenderLayers(stage).empty());
}

TEST(RenderLayerSave, GetSaveableRenderLayers_ExcludesDirtyLayerWithoutSavePermission)
{
    ScopedLayerEditorDCCFunctions guard;
    ScopedTempDir                 tempDir("le_render_layer_no_permission_test");

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto layer = PXR_NS::SdfLayer::CreateNew(tempDir.file("noPermission.usd"));
    layer->SetComment("edited");
    layer->SetPermissionToSave(false);
    stubRenderLayers({ layer });

    EXPECT_TRUE(Serialization::getSaveableRenderLayers(stage).empty());
}

// --- saveAnonymousLayer -------------------------------------------------------

namespace {

// Records what the save flow reported back to the DCC.
struct SaveCallRecord
{
    int         renderLayerSavedCalls { 0 };
    std::string oldIdentifier;
    std::string newIdentifier;
    int         updateRootLayerCalls { 0 };
};

void stubSaveCallRecording(
    SaveCallRecord&                            record,
    const std::vector<PXR_NS::SdfLayerRefPtr>& renderLayers)
{
    stubRenderLayers(renderLayers);
    RenderLayerFns renderLayer = layerEditorDCCFunctions().renderLayer;
    renderLayer.renderLayerSaved = [&record](
                                       const PXR_NS::UsdStageRefPtr&,
                                       const std::string& oldIdentifier,
                                       const std::string& newIdentifier) {
        ++record.renderLayerSavedCalls;
        record.oldIdentifier = oldIdentifier;
        record.newIdentifier = newIdentifier;
    };
    setRenderLayerFns(renderLayer);

    SerializationFns serialization;
    serialization.updateDCCObjectRootLayer
        = [&record](
              const std::string&,
              const std::string&,
              const PXR_NS::SdfLayerRefPtr&,
              bool,
              DccObjectRootLayerPathMode) { ++record.updateRootLayerCalls; };
    setSerializationFns(serialization);
}

} // namespace

TEST(RenderLayerSave, SaveAnonymousLayer_RenderLayer_RepointsRegistry)
{
    ScopedLayerEditorDCCFunctions guard;
    ScopedTempDir                 tempDir("le_render_layer_save_anon_test");

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto renderLayer = PXR_NS::SdfLayer::CreateAnonymous("RenderLayer_1");

    SaveCallRecord record;
    stubSaveCallRecording(record, { renderLayer });

    Serialization::PathInfo pathInfo;
    pathInfo.absolutePath = tempDir.file("renderLayer1.usd");

    Serialization::LayerParent parent;

    Serialization::saveAnonymousLayer(stage, renderLayer, pathInfo, parent);

    EXPECT_EQ(record.renderLayerSavedCalls, 1);
    EXPECT_EQ(record.oldIdentifier, renderLayer->GetIdentifier());
    EXPECT_NE(record.newIdentifier, record.oldIdentifier)
        << "the registry must be repointed away from the anonymous identifier";
    EXPECT_NE(record.newIdentifier.find("renderLayer1.usd"), std::string::npos);
    EXPECT_EQ(record.updateRootLayerCalls, 0)
        << "a render layer is never the DCC object's root layer";
}

// --- SaveLayersDialog ---------------------------------------------------------

class RenderLayerSaveDialogTest : public LayerEditorTestFixture
{
};

// Each row derives its suggested file from its own layer's number suffix, so a render
// layer and a plain anonymous layer whose names end in the same digit land on the same
// not-yet-existing path. okToSave would then reject the whole save.
TEST_F(RenderLayerSaveDialogTest, SaveLayersDialog_CollidingSuggestedPathsAreDisambiguated)
{
    auto renderLayer = PXR_NS::SdfLayer::CreateAnonymous("RenderLayer_0");
    stubRenderLayers({ renderLayer });

    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);

    std::set<std::string> paths;
    int                   rowCount = 0;
    for (auto* pathEdit : dlg.findChildren<QLineEdit*>()) {
        ++rowCount;
        paths.insert(pathEdit->text().toStdString());
    }

    EXPECT_GE(rowCount, 2) << "the stub stage's anonymous layer plus the render layer";
    EXPECT_EQ(paths.size(), static_cast<size_t>(rowCount)) << "suggested paths must be unique";
}

} // namespace UsdLayerEditor
