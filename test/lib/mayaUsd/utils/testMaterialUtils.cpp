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
#include <usdUfe/ufe/MaterialUtils.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/usd/usd/stage.h>

#include <gtest/gtest.h>

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

UsdStageRefPtr gTestStage;

UsdStageWeakPtr testStageAccessor(const Ufe::Path&) { return gTestStage; }

std::map<std::string, std::set<std::pair<std::string, std::string>>>
groupMaterialsFromRenderers(const std::multimap<std::string, Ufe::ContextItem>& materials)
{
    std::map<std::string, std::set<std::pair<std::string, std::string>>> grouped;
    for (const auto& entry : materials) {
        grouped[entry.first].emplace(entry.second.label, entry.second.item);
    }
    return grouped;
}

bool containsEntry(
    const std::map<std::string, std::set<std::pair<std::string, std::string>>>& grouped,
    const std::string&                                                          renderer,
    const std::string&                                                          label,
    const std::string&                                                          item)
{
    const auto rendererIt = grouped.find(renderer);
    if (rendererIt == grouped.end()) {
        return false;
    }
    return rendererIt->second.count(std::make_pair(label, item)) != 0;
}

} // namespace

class MaterialUtilsTest : public ::testing::Test
{
protected:
    void SetUp() override { UsdUfe::setStageAccessorFn(testStageAccessor); }

    void TearDown() override { gTestStage.Reset(); }

    void openTestScene(const char* fileName)
    {
        const std::string path = std::string(MATERIAL_TEST_SAMPLES_DIR) + "/" + fileName;
        gTestStage = UsdStage::Open(path);
        ASSERT_TRUE(gTestStage);
    }
};

TEST_F(MaterialUtilsTest, getMaterialsInStage)
{
    static const Ufe::Path dummyContextPath = {};

    openTestScene("multipleMaterials.usda");
    EXPECT_EQ(
        UsdUfe::getMaterialsInStage(dummyContextPath),
        std::vector<SdfPath>({
            SdfPath("/mtl/UsdPreviewSurface1"),
            SdfPath("/mtl/UsdPreviewSurface2"),
        }));

    openTestScene("singleMaterial.usda");
    EXPECT_EQ(
        UsdUfe::getMaterialsInStage(dummyContextPath),
        std::vector<SdfPath>({ SdfPath("/mtl/UsdPreviewSurface1") }));

    openTestScene("noMaterial.usda");
    EXPECT_TRUE(UsdUfe::getMaterialsInStage(dummyContextPath).empty());

    gTestStage.Reset();
    EXPECT_TRUE(UsdUfe::getMaterialsInStage(dummyContextPath).empty());
}

TEST(GetMaterialsFromRenderers, containsExpectedEntries)
{
    const auto grouped = groupMaterialsFromRenderers(UsdUfe::getMaterialsFromRenderers());

    EXPECT_TRUE(containsEntry(grouped, "USD", "USD Preview Surface", "UsdPreviewSurface"));
    EXPECT_TRUE(containsEntry(
        grouped, "MaterialX", "Standard Surface", "ND_standard_surface_surfaceshader"));
    EXPECT_TRUE(containsEntry(
        grouped, "MaterialX", "USD Preview Surface", "ND_UsdPreviewSurface_surfaceshader"));

    for (const auto& rendererEntry : grouped) {
        for (const auto& materialEntry : rendererEntry.second) {
            EXPECT_FALSE(materialEntry.first.empty());
            EXPECT_FALSE(materialEntry.second.empty());
        }
    }
}
