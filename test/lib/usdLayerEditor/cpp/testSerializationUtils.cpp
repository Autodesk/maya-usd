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
#include "utilSerialization.h"

#include <pxr/usd/sdf/layer.h>

#include <gtest/gtest.h>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {
namespace Serialization {

namespace {
bool sublayerPathsContain(const SdfLayerRefPtr& layer, const std::string& path)
{
    for (const auto& p : layer->GetSubLayerPaths()) {
        if (p == path)
            return true;
    }
    return false;
}
} // namespace

// ── ensureUSDFileExtension ────────────────────────────────────────────────────

TEST(SerializationUtils, EnsureUSDFileExtension_AppendsUsdWhenNoExtension)
{
    std::string path = "myfile";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.usd");
}

TEST(SerializationUtils, EnsureUSDFileExtension_NoOpForUsd)
{
    std::string path = "myfile.usd";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.usd");
}

TEST(SerializationUtils, EnsureUSDFileExtension_NoOpForUsdc)
{
    std::string path = "myfile.usdc";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.usdc");
}

TEST(SerializationUtils, EnsureUSDFileExtension_NoOpForUsda)
{
    std::string path = "myfile.usda";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.usda");
}

TEST(SerializationUtils, EnsureUSDFileExtension_NoOpForUsdz)
{
    std::string path = "myfile.usdz";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.usdz");
}

TEST(SerializationUtils, EnsureUSDFileExtension_AppendsUsdForForeignExtension)
{
    std::string path = "myfile.txt";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.txt.usd");
}

// ── updateSubLayer ────────────────────────────────────────────────────────────

TEST(SerializationUtils, UpdateSubLayer_NoOpForNullParent)
{
    auto sublayer = SdfLayer::CreateAnonymous("sub_noparent");
    EXPECT_NO_THROW(updateSubLayer(nullptr, sublayer, "/new/path.usd"));
}

TEST(SerializationUtils, UpdateSubLayer_NoOpForNullOldLayer)
{
    auto parent = SdfLayer::CreateAnonymous("parent_nosub");
    EXPECT_NO_THROW(updateSubLayer(parent, nullptr, "/new/path.usd"));
}

TEST(SerializationUtils, UpdateSubLayer_ReplacesIdentifierInParent)
{
    auto parent   = SdfLayer::CreateAnonymous("parent_upd");
    auto sublayer = SdfLayer::CreateAnonymous("sub_upd");

    parent->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
    ASSERT_TRUE(sublayerPathsContain(parent, sublayer->GetIdentifier()))
        << "precondition: sublayer identifier must be in parent's paths";

    const std::string newPath = "/new/layer.usd";
    updateSubLayer(parent, sublayer, newPath);

    // Old identifier removed; new path present.
    EXPECT_FALSE(sublayerPathsContain(parent, sublayer->GetIdentifier()));
    EXPECT_TRUE(sublayerPathsContain(parent, newPath));
}

TEST(SerializationUtils, UpdateSubLayer_NewParentHasNoSubLayerBecomesNoOp)
{
    auto parent   = SdfLayer::CreateAnonymous("parent_empty");
    auto sublayer = SdfLayer::CreateAnonymous("sub_notadded");
    // sublayer was never added to parent → Replace finds nothing → no crash, no change
    EXPECT_NO_THROW(updateSubLayer(parent, sublayer, "/new/layer.usd"));
    EXPECT_TRUE(parent->GetSubLayerPaths().empty());
}

} // namespace Serialization
} // namespace UsdLayerEditor
