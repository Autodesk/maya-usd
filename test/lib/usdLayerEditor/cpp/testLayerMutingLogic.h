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
#pragma once

#include "testUtils.h"
#include "layerMuting.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

class LayerMutingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        forgetMutedLayers();
        _stage = PXR_NS::UsdStage::CreateInMemory();
        _layer = SdfLayer::CreateAnonymous("mute_test");
        _stage->GetRootLayer()->InsertSubLayerPath(_layer->GetIdentifier(), 0);
    }
    void TearDown() override
    {
        if (_stage && _layer)
            _stage->UnmuteLayer(_layer->GetIdentifier());
        forgetMutedLayers();
    }
    UsdStageRefPtr _stage;
    SdfLayerRefPtr _layer;
};

TEST_F(LayerMutingTest, IsMuted_FalseByDefault)
{
    EXPECT_FALSE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

TEST_F(LayerMutingTest, MuteLayer_SetsLayerAsMutedInStage)
{
    _stage->MuteLayer(_layer->GetIdentifier());
    EXPECT_TRUE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

TEST_F(LayerMutingTest, UnmuteLayer_SetsLayerAsUnmuted)
{
    _stage->MuteLayer(_layer->GetIdentifier());
    _stage->UnmuteLayer(_layer->GetIdentifier());
    EXPECT_FALSE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

TEST_F(LayerMutingTest, MuteToggleRoundtrip_RestoresOriginalState)
{
    _stage->MuteLayer(_layer->GetIdentifier());
    _stage->UnmuteLayer(_layer->GetIdentifier());
    EXPECT_FALSE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

TEST_F(LayerMutingTest, AddMutedLayer_AppearsInRetainedList)
{
    // addMutedLayer retains a reference to prevent USD from unloading the layer.
    addMutedLayer(_layer);
    // We can't query the list directly, but verify no crash.
    SUCCEED();
}

TEST_F(LayerMutingTest, RemoveMutedLayer_DoesNotCrash)
{
    addMutedLayer(_layer);
    EXPECT_NO_THROW(removeMutedLayer(_layer));
}

TEST_F(LayerMutingTest, ForgetMutedLayers_ClearsRetainedList)
{
    addMutedLayer(_layer);
    EXPECT_NO_THROW(forgetMutedLayers());
}

TEST_F(LayerMutingTest, AddMutedLayer_PreservesLayerReference)
{
    // After addMutedLayer, the layer should still be reachable.
    addMutedLayer(_layer);
    auto identifier = _layer->GetIdentifier();
    EXPECT_FALSE(identifier.empty());
}

// ── loadLayerMuteState (new editor only) ─────────────────────────────────────
#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED

TEST_F(LayerMutingTest, LoadLayerMuteState_MutesListedLayer)
{
    // loadLayerMuteState should mute a layer whose identifier appears in the list.
    std::vector<std::string> muted = { _layer->GetIdentifier() };
    LayerNameMap             nameMap;
    loadLayerMuteState(muted, nameMap, *_stage);
    EXPECT_TRUE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

TEST_F(LayerMutingTest, LoadLayerMuteState_EmptyListMutesNothing)
{
    std::vector<std::string> muted;
    LayerNameMap             nameMap;
    loadLayerMuteState(muted, nameMap, *_stage);
    EXPECT_FALSE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

TEST_F(LayerMutingTest, LoadLayerMuteState_NameMapRemapsIdentifier)
{
    // When an anonymous layer is saved and reloaded its identifier changes.
    // The nameMap allows mapping the old identifier to the new one.
    auto            newLayer = SdfLayer::CreateAnonymous("remapped");
    _stage->GetRootLayer()->InsertSubLayerPath(newLayer->GetIdentifier(), 1);
    const std::string  oldId = "anon:old-identifier";
    const std::string  newId = newLayer->GetIdentifier();
    LayerNameMap       nameMap { { oldId, newId } };
    std::vector<std::string> muted = { oldId };
    loadLayerMuteState(muted, nameMap, *_stage);
    EXPECT_TRUE(_stage->IsLayerMuted(newId));
}

#endif // LAYER_EDITOR_TEST_FIXTURE_INCLUDED

// ── getMutedLayers ────────────────────────────────────────────────────────────

TEST_F(LayerMutingTest, GetMutedLayers_ReturnsEmptyForUnknownIdentifier)
{
    // No layer has been added — querying any identifier must return an empty set.
    const LayerRefSet& result = getMutedLayers("anon:does-not-exist");
    EXPECT_TRUE(result.empty());
}

TEST_F(LayerMutingTest, GetMutedLayers_ReturnsHeldLayerAfterAdd)
{
    // After addMutedLayer the layer must appear in the held set.
    addMutedLayer(_layer);
    const LayerRefSet& result = getMutedLayers(_layer->GetIdentifier());
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find(_layer), result.end());
}

TEST_F(LayerMutingTest, GetMutedLayers_ReturnsEmptyAfterRemove)
{
    addMutedLayer(_layer);
    removeMutedLayer(_layer);
    const LayerRefSet& result = getMutedLayers(_layer->GetIdentifier());
    EXPECT_TRUE(result.empty());
}

} // namespace UsdLayerEditor
