#pragma once
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
#include "customLayerData.h"
#ifndef MAYAUSD_OLD_LAYER_EDITOR
// Layers:: utilities live in UsdLayerEditorLib, which the old editor test binary
// does not link; only the guarded LayersTest cases below use them.
#include "layers.h"
#endif
#include "warningDialogs.h"

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/stage.h>

#include <QtCore/QStringList>
#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

// ── CustomLayerData ───────────────────────────────────────────────────────────

TEST(CustomLayerDataTest, SetGetStringArray_RoundTrip)
{
    auto layer = SdfLayer::CreateAnonymous("cld_arr");
    VtArray<std::string> data = {"alpha", "beta", "gamma"};
    CustomLayerData::setStringArray(data, layer, TfToken("myArr"));
    EXPECT_EQ(CustomLayerData::getStringArray(layer, TfToken("myArr")), data);
}

TEST(CustomLayerDataTest, GetStringArray_EmptyForAbsentKey)
{
    auto layer = SdfLayer::CreateAnonymous("cld_arr_missing");
    EXPECT_TRUE(CustomLayerData::getStringArray(layer, TfToken("noSuchKey")).empty());
}

TEST(CustomLayerDataTest, SetGetString_RoundTrip)
{
    auto layer = SdfLayer::CreateAnonymous("cld_str");
    CustomLayerData::setString("hello world", layer, TfToken("strKey"));
    EXPECT_EQ(CustomLayerData::getString(layer, TfToken("strKey")), "hello world");
}

TEST(CustomLayerDataTest, GetString_EmptyForAbsentKey)
{
    auto layer = SdfLayer::CreateAnonymous("cld_str_missing");
    EXPECT_EQ(CustomLayerData::getString(layer, TfToken("absent")), "");
}

TEST(CustomLayerDataTest, SetStringArray_EmptyArrayClearsKey)
{
    auto layer = SdfLayer::CreateAnonymous("cld_clear");
    VtArray<std::string> data = {"x"};
    CustomLayerData::setStringArray(data, layer, TfToken("k"));
    CustomLayerData::setStringArray(VtArray<std::string>{}, layer, TfToken("k"));
    EXPECT_TRUE(CustomLayerData::getStringArray(layer, TfToken("k")).empty());
}

// ── Layers ────────────────────────────────────────────────────────────────────
// Layers:: functions live in UsdLayerEditorLib which the old editor test binary
// does not link (ODR conflict risk with LEGACY_SOURCES). Guard until a shim exists.
#ifndef MAYAUSD_OLD_LAYER_EDITOR

TEST(LayersTest, GetLocalTargetLayerAsString_ReturnsSubLayerIdentifier)
{
    auto stage    = PXR_NS::UsdStage::CreateInMemory();
    auto sublayer = SdfLayer::CreateAnonymous("tgt_str");
    stage->GetRootLayer()->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
    stage->SetEditTarget(UsdEditTarget(sublayer));
    EXPECT_EQ(Layers::getLocalTargetLayerAsString(stage), sublayer->GetIdentifier());
}

TEST(LayersTest, GetLocalTargetLayerAsString_RootLayerIsLocal)
{
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    EXPECT_EQ(Layers::getLocalTargetLayerAsString(stage), stage->GetRootLayer()->GetIdentifier());
}

TEST(LayersTest, GetLocalTargetLayerFromString_FindsByIdentifier)
{
    auto stage    = PXR_NS::UsdStage::CreateInMemory();
    auto sublayer = SdfLayer::CreateAnonymous("find_by_id");
    stage->GetRootLayer()->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
    Layers::LayerNameMap nameMap;
    auto result = Layers::getLocalTargetLayerFromString(nameMap, *stage, sublayer->GetIdentifier());
    EXPECT_TRUE(result);
    EXPECT_EQ(result->GetIdentifier(), sublayer->GetIdentifier());
}

TEST(LayersTest, GetLocalTargetLayerFromString_EmptyIdentifierReturnsNull)
{
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    Layers::LayerNameMap nameMap;
    EXPECT_FALSE(Layers::getLocalTargetLayerFromString(nameMap, *stage, ""));
}

TEST(LayersTest, GetLocalTargetLayerFromString_NameMapRemapsIdentifier)
{
    auto stage    = PXR_NS::UsdStage::CreateInMemory();
    auto layer    = SdfLayer::CreateAnonymous("remap_target");
    stage->GetRootLayer()->InsertSubLayerPath(layer->GetIdentifier(), 0);
    const std::string    oldId   = "anon:old-target-id";
    Layers::LayerNameMap nameMap = { { oldId, layer->GetIdentifier() } };
    auto result = Layers::getLocalTargetLayerFromString(nameMap, *stage, oldId);
    EXPECT_TRUE(result);
    EXPECT_EQ(result->GetIdentifier(), layer->GetIdentifier());
}

TEST(LayersTest, GetLocalTargetLayerFromString_UnknownIdentifierReturnsNull)
{
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    Layers::LayerNameMap nameMap;
    EXPECT_FALSE(Layers::getLocalTargetLayerFromString(nameMap, *stage, "anon:nonexistent"));
}

#endif // !MAYAUSD_OLD_LAYER_EDITOR

// ── warningDialogs ────────────────────────────────────────────────────────────
// Old editor's confirmDialog/warningDialog take QWidget* as first arg; new
// editor's signatures are (const QString&, const QString&, ...) — guard these.
#ifndef MAYAUSD_OLD_LAYER_EDITOR

TEST(WarningDialogsTest, ConfirmDialog_HandlerReturnsTrueReturnsTrue)
{
    auto prev = setModalDialogTestHandler([](const QString&, const QString&) { return true; });
    bool result = confirmDialog("Title", "Message");
    setModalDialogTestHandler(prev);
    EXPECT_TRUE(result);
}

TEST(WarningDialogsTest, ConfirmDialog_HandlerReturnsFalseReturnsFalse)
{
    auto prev = setModalDialogTestHandler([](const QString&, const QString&) { return false; });
    bool result = confirmDialog("Title", "Message");
    setModalDialogTestHandler(prev);
    EXPECT_FALSE(result);
}

TEST(WarningDialogsTest, WarningDialog_HandlerIsCalled)
{
    int count = 0;
    auto prev = setModalDialogTestHandler([&](const QString&, const QString&) {
        ++count; return true;
    });
    warningDialog("W Title", "W Message");
    setModalDialogTestHandler(prev);
    EXPECT_EQ(count, 1);
}

TEST(WarningDialogsTest, ConfirmDialog_WithBulletList_HandlerCalled)
{
    int count = 0;
    auto prev = setModalDialogTestHandler([&](const QString&, const QString&) {
        ++count; return true;
    });
    QStringList bullets = { "layer1.usd", "layer2.usd" };
    confirmDialog("Title", "Msg", &bullets);
    setModalDialogTestHandler(prev);
    EXPECT_EQ(count, 1);
}

TEST(WarningDialogsTest, SetModalDialogTestHandler_ReturnsPrevious)
{
    ModalDialogTestHandler h1 = [](const QString&, const QString&) { return true; };
    auto orig = setModalDialogTestHandler(h1);   // capture original
    ModalDialogTestHandler h2 = [](const QString&, const QString&) { return false; };
    auto prev2 = setModalDialogTestHandler(h2);
    EXPECT_TRUE(static_cast<bool>(prev2));        // prev2 should be h1
    setModalDialogTestHandler(orig);              // restore original
}

#endif // !MAYAUSD_OLD_LAYER_EDITOR

} // namespace UsdLayerEditor
