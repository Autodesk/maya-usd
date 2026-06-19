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
#include "layerLocking.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <gtest/gtest.h>

#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

class LayerLockingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        forgetLockedLayers();
        forgetSystemLockedLayers();
        _layer = SdfLayer::CreateAnonymous("lock_test");
    }
    void TearDown() override
    {
        if (_layer) {
            _layer->SetPermissionToEdit(true);
            _layer->SetPermissionToSave(true);
        }
        forgetLockedLayers();
        forgetSystemLockedLayers();
    }
    SdfLayerRefPtr _layer;
};

TEST_F(LayerLockingTest, IsLayerLocked_FalseByDefault)
{
    EXPECT_FALSE(isLayerLocked(_layer));
}

TEST_F(LayerLockingTest, LockLayer_SetsLayerAsLocked)
{
    lockLayer("", _layer, LayerLock_Locked, /*updateDCCAttr=*/false);
    EXPECT_TRUE(isLayerLocked(_layer));
}

TEST_F(LayerLockingTest, UnlockLayer_SetsLayerAsUnlocked)
{
    lockLayer("", _layer, LayerLock_Locked, false);
    lockLayer("", _layer, LayerLock_Unlocked, false);
    EXPECT_FALSE(isLayerLocked(_layer));
}

TEST_F(LayerLockingTest, LockLayer_RevokesPermissionToEdit)
{
    lockLayer("", _layer, LayerLock_Locked, false);
    EXPECT_FALSE(_layer->PermissionToEdit());
}

TEST_F(LayerLockingTest, UnlockLayer_RestoresPermissionToEdit)
{
    lockLayer("", _layer, LayerLock_Locked, false);
    lockLayer("", _layer, LayerLock_Unlocked, false);
    EXPECT_TRUE(_layer->PermissionToEdit());
}

TEST_F(LayerLockingTest, LockLayer_ToggleRoundtrip_RestoresOriginalState)
{
    // Lock then unlock: layer must be back to unlocked.
    lockLayer("", _layer, LayerLock_Locked, false);
    lockLayer("", _layer, LayerLock_Unlocked, false);
    EXPECT_FALSE(isLayerLocked(_layer));
    EXPECT_TRUE(_layer->PermissionToEdit());
}

TEST_F(LayerLockingTest, SystemLockLayer_SetsSystemLocked)
{
    lockLayer("", _layer, LayerLock_SystemLocked, false);
    EXPECT_TRUE(isLayerSystemLocked(_layer));
}

TEST_F(LayerLockingTest, SystemLockLayer_RevokesPermissionToEdit)
{
    lockLayer("", _layer, LayerLock_SystemLocked, false);
    EXPECT_FALSE(_layer->PermissionToEdit());
    EXPECT_TRUE(isLayerSystemLocked(_layer));
}

TEST_F(LayerLockingTest, ForgetLockedLayers_ClearsAllState)
{
    lockLayer("", _layer, LayerLock_Locked, false);
    ASSERT_TRUE(isLayerLocked(_layer));
    forgetLockedLayers();
    EXPECT_FALSE(isLayerLocked(_layer));
}

TEST_F(LayerLockingTest, AddLockedLayer_AppearsInLockedList)
{
    addLockedLayer(_layer);
    EXPECT_TRUE(isLayerLocked(_layer));
}

TEST_F(LayerLockingTest, RemoveLockedLayer_DisappearsFromLockedList)
{
    addLockedLayer(_layer);
    removeLockedLayer(_layer);
    EXPECT_FALSE(isLayerLocked(_layer));
}

TEST_F(LayerLockingTest, AddSystemLockedLayer_AppearsInSystemLockedList)
{
    addSystemLockedLayer(_layer);
    EXPECT_TRUE(isLayerSystemLocked(_layer));
}

TEST_F(LayerLockingTest, ForgetSystemLockedLayers_ClearsSystemLockedList)
{
    addSystemLockedLayer(_layer);
    forgetSystemLockedLayers();
    EXPECT_FALSE(isLayerSystemLocked(_layer));
}

// ── getLockedLayersIdentifiers + loadLayerLockState (new editor only) ────────
#ifndef MAYAUSD_OLD_LAYER_EDITOR

TEST_F(LayerLockingTest, GetLockedLayersIdentifiers_EmptyWhenNoneAdded)
{
    std::vector<std::string> ids = getLockedLayersIdentifiers();
    EXPECT_TRUE(ids.empty());
}

TEST_F(LayerLockingTest, GetLockedLayersIdentifiers_ContainsIdentifierAfterLock)
{
    lockLayer("", _layer, LayerLock_Locked, false);
    auto ids = getLockedLayersIdentifiers();
    auto it = std::find(ids.begin(), ids.end(), _layer->GetIdentifier());
    EXPECT_NE(it, ids.end());
}

TEST_F(LayerLockingTest, GetLockedLayersIdentifiers_EmptyAfterForget)
{
    lockLayer("", _layer, LayerLock_Locked, false);
    forgetLockedLayers();
    EXPECT_TRUE(getLockedLayersIdentifiers().empty());
}

TEST_F(LayerLockingTest, LoadLayerLockState_LocksListedLayer)
{
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    stage->GetRootLayer()->InsertSubLayerPath(_layer->GetIdentifier(), 0);
    std::vector<std::string> locked = { _layer->GetIdentifier() };
    LayerNameMap             nameMap;
    loadLayerLockState(locked, nameMap, *stage);
    EXPECT_TRUE(isLayerLocked(_layer));
}

TEST_F(LayerLockingTest, LoadLayerLockState_EmptyListLocksNothing)
{
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    stage->GetRootLayer()->InsertSubLayerPath(_layer->GetIdentifier(), 0);
    std::vector<std::string> locked;
    LayerNameMap             nameMap;
    loadLayerLockState(locked, nameMap, *stage);
    EXPECT_FALSE(isLayerLocked(_layer));
}

TEST_F(LayerLockingTest, LoadLayerLockState_NameMapRemapsIdentifier)
{
    auto              stage    = PXR_NS::UsdStage::CreateInMemory();
    auto              newLayer = SdfLayer::CreateAnonymous("remap_lock");
    stage->GetRootLayer()->InsertSubLayerPath(newLayer->GetIdentifier(), 0);
    const std::string        oldId   = "anon:old-lock-id";
    const std::string        newId   = newLayer->GetIdentifier();
    LayerNameMap             nameMap { { oldId, newId } };
    std::vector<std::string> locked  = { oldId };
    loadLayerLockState(locked, nameMap, *stage);
    EXPECT_TRUE(isLayerLocked(newLayer));
}

#endif // MAYAUSD_OLD_LAYER_EDITOR

} // namespace UsdLayerEditor
