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
#include "layers.h"
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
    auto                 layer = SdfLayer::CreateAnonymous("cld_arr");
    VtArray<std::string> data = { "alpha", "beta", "gamma" };
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
    auto                 layer = SdfLayer::CreateAnonymous("cld_clear");
    VtArray<std::string> data = { "x" };
    CustomLayerData::setStringArray(data, layer, TfToken("k"));
    CustomLayerData::setStringArray(VtArray<std::string> {}, layer, TfToken("k"));
    EXPECT_TRUE(CustomLayerData::getStringArray(layer, TfToken("k")).empty());
}

// ── Layers ────────────────────────────────────────────────────────────────────

// ── warningDialogs ────────────────────────────────────────────────────────────

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
    int  count = 0;
    auto prev = setModalDialogTestHandler([&](const QString&, const QString&) {
        ++count;
        return true;
    });
    warningDialog("W Title", "W Message");
    setModalDialogTestHandler(prev);
    EXPECT_EQ(count, 1);
}

TEST(WarningDialogsTest, SetModalDialogTestHandler_ReturnsPrevious)
{
    ModalDialogTestHandler h1 = [](const QString&, const QString&) { return true; };
    auto                   orig = setModalDialogTestHandler(h1); // capture original
    ModalDialogTestHandler h2 = [](const QString&, const QString&) { return false; };
    auto                   prev2 = setModalDialogTestHandler(h2);
    EXPECT_TRUE(static_cast<bool>(prev2)); // prev2 should be h1
    setModalDialogTestHandler(orig);       // restore original
}

} // namespace UsdLayerEditor
