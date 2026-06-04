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

} // namespace UsdLayerEditor
