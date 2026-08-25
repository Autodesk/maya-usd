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

#include <mayaUsd/nodes/sceneRenderDescription.h>
#include <mayaUsdUI/ui/mayaRendererProvider.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace {

// Exposes the protected switchRenderer() so edge cases (empty name, unknown
// name) can be exercised directly, independently of requestRenderer()'s own
// "already current" guard.
class TestableRendererProvider : public MayaUsdRenderSetup::MayaRendererProvider
{
public:
    using MayaUsdRenderSetup::MayaRendererProvider::switchRenderer;
};

std::string getLegacyCurrentRenderer()
{
    MString value;
    MGlobal::executeCommand("getAttr defaultRenderGlobals.currentRenderer", value);
    return value.asChar();
}

void setLegacyCurrentRenderer(const std::string& name)
{
    MString cmd;
    cmd.format(
        "setAttr -type \"string\" defaultRenderGlobals.currentRenderer \"^1s\"",
        MString(name.c_str()));
    MGlobal::executeCommand(cmd);
}

void setHydraCurrentRenderer(const std::string& name)
{
    MAYAUSD_NS_DEF::SceneRenderDescription::setCurrentRenderer(name);
}

using RendererInfo = AdskUsdRenderSetup::RendererInfo;

std::vector<RendererInfo>::const_iterator
findByHydra(const std::vector<RendererInfo>& renderers, bool isHydra)
{
    return std::find_if(renderers.begin(), renderers.end(), [isHydra](const RendererInfo& r) {
        return r.isHydra == isHydra;
    });
}

// Used where the test only cares about the defaultRenderGlobals.currentRenderer
// plug's raw value, not about the name being one MayaRendererProvider actually
// knows about (i.e. not exercising the isHydra-classification branches, which
// require a name present in availableRenderers()).
const char* const kArbitraryRendererName = "some-arbitrary-renderer-name";

} // namespace

class MayaRendererProviderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _origLegacyRenderer = getLegacyCurrentRenderer();
        _origHydraRenderer = MAYAUSD_NS_DEF::SceneRenderDescription::getCurrentRenderer();
    }

    void TearDown() override
    {
        setLegacyCurrentRenderer(_origLegacyRenderer);
        setHydraCurrentRenderer(_origHydraRenderer);
    }

    MayaUsdRenderSetup::MayaRendererProvider provider;

private:
    std::string _origLegacyRenderer;
    std::string _origHydraRenderer;
};

TEST_F(MayaRendererProviderTest, AvailableRenderersIsNonEmpty)
{
    const auto renderers = provider.availableRenderers();
    ASSERT_FALSE(renderers.empty());

    for (const auto& r : renderers) {
        EXPECT_FALSE(r.name.empty());
    }
}

TEST_F(MayaRendererProviderTest, AvailableRenderersHasNoDuplicateNames)
{
    const auto renderers = provider.availableRenderers();

    std::set<std::string> names;
    for (const auto& r : renderers) {
        EXPECT_TRUE(names.insert(r.name).second) << "Duplicate renderer name: " << r.name;
    }
}

TEST_F(MayaRendererProviderTest, CurrentRendererMatchesDefaultRenderGlobalsWhenSet)
{
    setLegacyCurrentRenderer(kArbitraryRendererName);
    setHydraCurrentRenderer("");

    EXPECT_EQ(provider.currentRenderer(), kArbitraryRendererName);
}

TEST_F(MayaRendererProviderTest, CurrentRendererFallsBackToSceneRenderDescriptionWhenLegacyEmpty)
{
    const auto renderers = provider.availableRenderers();
    const auto it = findByHydra(renderers, /*isHydra*/ true);
    if (it == renderers.end()) {
        GTEST_SKIP() << "No Hydra renderer plugins registered in this environment.";
    }

    setLegacyCurrentRenderer("");
    setHydraCurrentRenderer(it->name);

    EXPECT_EQ(provider.currentRenderer(), it->name);
}

TEST_F(MayaRendererProviderTest, CurrentRendererIsEmptyWhenBothStoresAreEmpty)
{
    setLegacyCurrentRenderer("");
    setHydraCurrentRenderer("");

    EXPECT_TRUE(provider.currentRenderer().empty());
}

TEST_F(MayaRendererProviderTest, RequestRendererToHydraRendererUpdatesSceneRenderDescription)
{
    const auto renderers = provider.availableRenderers();
    const auto it = findByHydra(renderers, /*isHydra*/ true);
    if (it == renderers.end()) {
        GTEST_SKIP() << "No Hydra renderer plugins registered in this environment.";
    }

    setLegacyCurrentRenderer("");
    setHydraCurrentRenderer("");

    provider.requestRenderer(it->name);

    EXPECT_EQ(provider.currentRenderer(), it->name);
    EXPECT_EQ(MAYAUSD_NS_DEF::SceneRenderDescription::getCurrentRenderer(), it->name);
}

TEST_F(MayaRendererProviderTest, RequestRendererToLegacyRendererClearsSceneRenderDescription)
{
    const auto renderers = provider.availableRenderers();
    const auto legacyIt = findByHydra(renderers, /*isHydra*/ false);
    if (legacyIt == renderers.end()) {
        GTEST_SKIP() << "No legacy (non-Hydra) renderers registered in this environment.";
    }

    // Seed a non-empty Hydra renderer first so we can verify the switch clears it.
    const auto hydraIt = findByHydra(renderers, /*isHydra*/ true);
    if (hydraIt != renderers.end()) {
        setHydraCurrentRenderer(hydraIt->name);
    } else {
        GTEST_SKIP() << "No Hydra renderers registered in this environment.";
    }

    provider.requestRenderer(legacyIt->name);

    EXPECT_EQ(provider.currentRenderer(), legacyIt->name);
    EXPECT_TRUE(MAYAUSD_NS_DEF::SceneRenderDescription::getCurrentRenderer().empty());
}

TEST_F(MayaRendererProviderTest, RequestRendererWithUnknownNameLeavesCurrentRendererUnchanged)
{
    setLegacyCurrentRenderer(kArbitraryRendererName);
    setHydraCurrentRenderer("");

    provider.requestRenderer("totally-bogus-renderer-name-xyz");

    EXPECT_EQ(provider.currentRenderer(), kArbitraryRendererName);
}

TEST_F(MayaRendererProviderTest, RequestRendererAlreadyCurrentIsNoOp)
{
    setLegacyCurrentRenderer(kArbitraryRendererName);
    // Sentinel: a legacy switch would clear this. If requestRenderer correctly
    // treats "already current" as a no-op, switchRenderer never runs and this
    // survives untouched.
    setHydraCurrentRenderer("sentinel-value");

    provider.requestRenderer(kArbitraryRendererName);

    EXPECT_EQ(provider.currentRenderer(), kArbitraryRendererName);
    EXPECT_EQ(MAYAUSD_NS_DEF::SceneRenderDescription::getCurrentRenderer(), "sentinel-value");
}

TEST_F(MayaRendererProviderTest, SwitchRendererWithEmptyNameIsNoOp)
{
    setLegacyCurrentRenderer(kArbitraryRendererName);
    setHydraCurrentRenderer("sentinel-value");

    TestableRendererProvider testable;
    testable.switchRenderer("");

    EXPECT_EQ(getLegacyCurrentRenderer(), kArbitraryRendererName);
    EXPECT_EQ(MAYAUSD_NS_DEF::SceneRenderDescription::getCurrentRenderer(), "sentinel-value");
}

TEST_F(MayaRendererProviderTest, SwitchRendererWithUnknownNameDoesNotTouchSceneRenderDescription)
{
    setHydraCurrentRenderer("sentinel-value");

    TestableRendererProvider testable;
    testable.switchRenderer("totally-bogus-renderer-name-xyz");

    // An unknown name is not found in the internal renderer map, so the
    // isHydra-branch that writes/clears SceneRenderDescription never runs.
    EXPECT_EQ(MAYAUSD_NS_DEF::SceneRenderDescription::getCurrentRenderer(), "sentinel-value");
}
