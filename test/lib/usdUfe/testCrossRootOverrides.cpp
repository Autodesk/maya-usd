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
#include <usdUfe/utils/usdUtils.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#if PXR_VERSION >= 2311
#include <pxr/usd/sdf/pathExpression.h>
#endif
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/variantSets.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

const SdfPath kDestPrimPath("/Root");

SdfLayerRefPtr makeLayer(const std::string& content)
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    EXPECT_TRUE(layer->ImportFromString("#usda 1.0\n" + content));
    return layer;
}

// The source stage, and a destination stage that references each of its root prims
// under /Root, like the export of a proxy shape.
struct Fixture
{
    explicit Fixture(const std::string& content, UsdStage::InitialLoadSet load = UsdStage::LoadAll)
        : sourceLayer(makeLayer(content))
        , sourceStage(UsdStage::Open(sourceLayer, load))
        , rootPrims(UsdUfe::getRootPrimsToReference(*sourceStage))
        , destStage(UsdStage::CreateInMemory())
    {
        destStage->DefinePrim(kDestPrimPath);
        for (const UsdPrim& root : rootPrims) {
            destStage->OverridePrim(kDestPrimPath.AppendChild(root.GetName()))
                .GetReferences()
                .AddReference(sourceLayer->GetIdentifier(), root.GetPath());
        }
    }

    UsdUfe::CrossRootOverridesResult author()
    {
        return UsdUfe::authorCrossRootOverrides(
            *sourceStage, rootPrims, destStage->GetRootLayer(), kDestPrimPath);
    }

    SdfPathVector targets(const char* relPath) const
    {
        SdfPathVector result;
        destStage->GetRelationshipAtPath(SdfPath(relPath)).GetTargets(&result);
        return result;
    }

    SdfPathVector connections(const char* attrPath) const
    {
        SdfPathVector result;
        destStage->GetAttributeAtPath(SdfPath(attrPath)).GetConnections(&result);
        return result;
    }

    SdfLayerRefPtr       sourceLayer;
    UsdStageRefPtr       sourceStage;
    std::vector<UsdPrim> rootPrims;
    UsdStageRefPtr       destStage;
};

} // namespace

TEST(CrossRootOverrides, getRootPrimsToReferenceSkipsOversAndClasses)
{
    Fixture fixture(R"(
def "Geo"
{
}
over "Overs"
{
}
class "_class_Geo"
{
}
def "Materials"
{
}
)");

    std::vector<SdfPath> paths;
    for (const UsdPrim& prim : fixture.rootPrims)
        paths.push_back(prim.GetPath());

    EXPECT_EQ(paths, (std::vector<SdfPath> { SdfPath("/Geo"), SdfPath("/Materials") }));
}

TEST(CrossRootOverrides, noOverrideWithinRootPrims)
{
    Fixture fixture(R"(
def "Geo"
{
    def "Cube"
    {
        rel material:binding = </Geo/Red>
    }
    def "Red"
    {
    }
}
def "Materials"
{
}
)");

    const UsdUfe::CrossRootOverridesResult result = fixture.author();

    EXPECT_EQ(result.nbOverrides, 0u);
    EXPECT_TRUE(result.failedOverrides.empty());
    EXPECT_FALSE(fixture.destStage->GetRootLayer()->GetPrimAtPath(SdfPath("/Root/Geo/Cube")));
    EXPECT_EQ(
        fixture.targets("/Root/Geo/Cube.material:binding"),
        (SdfPathVector { SdfPath("/Root/Geo/Red") }));
}

TEST(CrossRootOverrides, relationshipToAnotherRootPrim)
{
    Fixture fixture(R"(
def "Geo"
{
    def "Cube"
    {
        rel material:binding = </Materials/Red>
        rel proxyPrim = [</Materials/Red>, </Geo/Local>]
    }
    def "Local"
    {
    }
}
def "Materials"
{
    def "Red"
    {
    }
}
)");

    const UsdUfe::CrossRootOverridesResult result = fixture.author();

    EXPECT_EQ(result.nbOverrides, 2u);
    EXPECT_TRUE(result.unresolvedTargets.empty());
    EXPECT_TRUE(result.failedOverrides.empty());
    EXPECT_EQ(
        fixture.targets("/Root/Geo/Cube.material:binding"),
        (SdfPathVector { SdfPath("/Root/Materials/Red") }));
    // Targets within the same root prim are kept, since the explicit list replaces
    // the referenced targets.
    EXPECT_EQ(
        fixture.targets("/Root/Geo/Cube.proxyPrim"),
        (SdfPathVector { SdfPath("/Root/Materials/Red"), SdfPath("/Root/Geo/Local") }));

    // The override is an over, so the referenced prim keeps its type and specifier.
    const SdfPrimSpecHandle spec
        = fixture.destStage->GetRootLayer()->GetPrimAtPath(SdfPath("/Root/Geo/Cube"));
    ASSERT_TRUE(spec);
    EXPECT_EQ(spec->GetSpecifier(), SdfSpecifierOver);
}

TEST(CrossRootOverrides, connectionToAnotherRootPrim)
{
    Fixture fixture(R"(
def "Materials"
{
    def "Red"
    {
        token outputs:surface.connect = </NodeGraphs/Graph.outputs:out>
    }
}
def "NodeGraphs"
{
    def "Graph"
    {
        token outputs:out
    }
}
)");

    const UsdUfe::CrossRootOverridesResult result = fixture.author();

    EXPECT_EQ(result.nbOverrides, 1u);
    EXPECT_EQ(
        fixture.connections("/Root/Materials/Red.outputs:surface"),
        (SdfPathVector { SdfPath("/Root/NodeGraphs/Graph.outputs:out") }));
}

TEST(CrossRootOverrides, targetsThatAreNotReferencedAreDropped)
{
    Fixture fixture(R"(
def "Geo"
{
    def "Cube"
    {
        rel material:binding = [</Materials/Red>, </_class_Materials/Blue>]
    }
}
def "Materials"
{
    def "Red"
    {
    }
}
class "_class_Materials"
{
    def "Blue"
    {
    }
}
)");

    const UsdUfe::CrossRootOverridesResult result = fixture.author();

    EXPECT_EQ(result.nbOverrides, 1u);
    EXPECT_EQ(result.unresolvedTargets, (std::set<SdfPath> { SdfPath("/_class_Materials/Blue") }));
    EXPECT_EQ(
        fixture.targets("/Root/Geo/Cube.material:binding"),
        (SdfPathVector { SdfPath("/Root/Materials/Red") }));
}

TEST(CrossRootOverrides, instancesAreMadeNonInstanceable)
{
    Fixture fixture(R"(
def "Geo"
{
    def "Instance" (
        instanceable = true
        inherits = </_class_Instance>
    )
    {
    }
}
class "_class_Instance"
{
    def "Mesh"
    {
        rel material:binding = </Materials/Red>
    }
}
def "Materials"
{
    def "Red"
    {
    }
}
)");

    ASSERT_TRUE(
        fixture.sourceStage->GetPrimAtPath(SdfPath("/Geo/Instance/Mesh")).IsInstanceProxy());

    const UsdUfe::CrossRootOverridesResult result = fixture.author();

    EXPECT_EQ(result.nbOverrides, 1u);
    EXPECT_EQ(result.deinstancedPrims, (std::set<SdfPath> { SdfPath("/Geo/Instance") }));
    EXPECT_FALSE(fixture.destStage->GetPrimAtPath(SdfPath("/Root/Geo/Instance")).IsInstance());
    EXPECT_EQ(
        fixture.targets("/Root/Geo/Instance/Mesh.material:binding"),
        (SdfPathVector { SdfPath("/Root/Materials/Red") }));
}

TEST(CrossRootOverrides, overridesGoInsideTheirVariant)
{
    Fixture fixture(R"(
def "Geo" (
    variants = { string look = "red" }
    prepend variantSets = "look"
)
{
    def "Cube"
    {
    }
    def "LocalBlue"
    {
    }
    variantSet "look" = {
        "red" {
            over "Cube"
            {
                rel material:binding = </Materials/Red>
            }
        }
        "blue" {
            over "Cube"
            {
                rel material:binding = </Geo/LocalBlue>
            }
        }
    }
}
def "Materials"
{
    def "Red"
    {
    }
}
)");

    const UsdUfe::CrossRootOverridesResult result = fixture.author();

    EXPECT_EQ(result.nbOverrides, 1u);
    EXPECT_TRUE(result.overridesOutsideVariants.empty());
    EXPECT_TRUE(
        fixture.destStage->GetRootLayer()->GetPrimAtPath(SdfPath("/Root/Geo{look=red}Cube")));
    EXPECT_EQ(
        fixture.targets("/Root/Geo/Cube.material:binding"),
        (SdfPathVector { SdfPath("/Root/Materials/Red") }));

    // The other variant keeps its own binding.
    fixture.destStage->GetPrimAtPath(SdfPath("/Root/Geo"))
        .GetVariantSet("look")
        .SetVariantSelection("blue");
    EXPECT_EQ(
        fixture.targets("/Root/Geo/Cube.material:binding"),
        (SdfPathVector { SdfPath("/Root/Geo/LocalBlue") }));
}

TEST(CrossRootOverrides, unloadedPrimsAreReported)
{
    SdfLayerRefPtr payload = makeLayer(R"(
def "Heavy"
{
    def "Cube"
    {
        rel material:binding = </Materials/Red>
    }
}
)");

    Fixture fixture(
        R"(
def "Geo"
{
    def "Heavy" (
        prepend payload = @)"
            + payload->GetIdentifier() + R"(@</Heavy>
    )
    {
    }
}
def "Materials"
{
    def "Red"
    {
    }
}
)",
        UsdStage::LoadNone);

    const UsdUfe::CrossRootOverridesResult result = fixture.author();

    EXPECT_EQ(result.nbOverrides, 0u);
    EXPECT_EQ(result.unloadedPrims, (SdfPathVector { SdfPath("/Geo/Heavy") }));
}

#if PXR_VERSION >= 2311
TEST(CrossRootOverrides, pathExpressionToAnotherRootPrim)
{
    Fixture fixture(R"(
def "Geo"
{
    def "Cube"
    {
    }
}
def "Lights"
{
    def "Key"
    {
        pathExpression collection:lightLink:membershipExpression = "/Geo/Cube"
    }
}
)");

    const UsdUfe::CrossRootOverridesResult result = fixture.author();

    EXPECT_EQ(result.nbOverrides, 1u);

    SdfPathExpression expression;
    ASSERT_TRUE(fixture.destStage
                    ->GetAttributeAtPath(
                        SdfPath("/Root/Lights/Key.collection:lightLink:membershipExpression"))
                    .Get(&expression));
    EXPECT_EQ(expression.GetText(), "/Root/Geo/Cube");
}
#endif

TEST(CrossRootOverrides, nullDestinationLayerDoesNothing)
{
    Fixture fixture(R"(
def "Geo"
{
    def "Cube"
    {
        rel material:binding = </Materials/Red>
    }
}
def "Materials"
{
    def "Red"
    {
    }
}
)");

    const UsdUfe::CrossRootOverridesResult result = UsdUfe::authorCrossRootOverrides(
        *fixture.sourceStage, fixture.rootPrims, SdfLayerHandle(), kDestPrimPath);

    EXPECT_EQ(result.nbOverrides, 0u);
    EXPECT_TRUE(result.failedOverrides.empty());
}
