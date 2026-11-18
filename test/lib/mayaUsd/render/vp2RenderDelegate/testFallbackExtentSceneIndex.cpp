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
#include <mayaUsd/render/vp2RenderDelegate/sceneIndex/fallbackExtentSceneIndex.h>

#include <pxr/imaging/hd/extentSchema.h>
#include <pxr/imaging/hd/primvarSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/sceneIndexPrimView.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdsi/implicitSurfaceSceneIndex.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/boundable.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/modelAPI.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Every implicit type becomes a mesh, as the producer configures it.
HdContainerDataSourceHandle implicitSurfaceArgs()
{
    static const HdDataSourceBaseHandle toMesh = HdRetainedTypedSampledDataSource<TfToken>::New(
        HdsiImplicitSurfaceSceneIndexTokens->toMesh);

    return HdRetainedContainerDataSource::New(
        HdPrimTypeTokens->sphere,
        toMesh,
        HdPrimTypeTokens->cube,
        toMesh,
        HdPrimTypeTokens->cone,
        toMesh,
        HdPrimTypeTokens->cylinder,
        toMesh,
        HdPrimTypeTokens->capsule,
        toMesh,
        HdPrimTypeTokens->plane,
        toMesh);
}

// The producer's chain from the stage scene index to what the render index is given, up to the
// implicit surface conversion: the fallback extent scene index where the producer installs it,
// ahead of the extent resolution, instancing and the draw mode scene index.
struct Chain
{
    explicit Chain(const UsdStageRefPtr& stage_)
        : stage(stage_)
    {
        UsdImagingCreateSceneIndicesInfo info;
        info.overridesSceneIndexCallback
            = [this](const HdSceneIndexBaseRefPtr& input) -> HdSceneIndexBaseRefPtr {
            fallback = HdVP2FallbackExtentSceneIndex::New(input);
            return fallback;
        };

        const UsdImagingSceneIndices indices = UsdImagingCreateSceneIndices(info);
        stageSceneIndex = indices.stageSceneIndex;
        end = HdsiImplicitSurfaceSceneIndex::New(indices.finalSceneIndex, implicitSurfaceArgs());

        // In the producer's order: before the stage scene index, so the first prim has its extent.
        fallback->SetStage(stage);
        stageSceneIndex->SetStage(stage);
    }

    void setTime(double time) { stageSceneIndex->SetTime(UsdTimeCode(time)); }

    // The extent the render index would read for the prim: an empty range if it has none, as
    // HdSceneIndexAdapterSceneDelegate::GetExtent gives.
    GfRange3d extent(const SdfPath& path) const
    {
        const HdSceneIndexPrim        prim = end->GetPrim(path);
        const HdExtentSchema          schema = HdExtentSchema::GetFromParent(prim.dataSource);
        const HdVec3dDataSourceHandle min = schema.GetMin();
        const HdVec3dDataSourceHandle max = schema.GetMax();
        if (!min || !max) {
            return GfRange3d();
        }
        return GfRange3d(min->GetTypedValue(0.0f), max->GetTypedValue(0.0f));
    }

    // The box the points of the prim span, or an empty range if it has none.
    GfRange3d pointsRange(const SdfPath& path) const
    {
        const HdSceneIndexPrim             prim = end->GetPrim(path);
        const HdVec3fArrayDataSourceHandle points
            = HdVec3fArrayDataSource::Cast(HdPrimvarsSchema::GetFromParent(prim.dataSource)
                                               .GetPrimvar(HdTokens->points)
                                               .GetPrimvarValue());

        GfRange3d range;
        if (points) {
            for (const GfVec3f& point : points->GetTypedValue(0.0f)) {
                range.UnionWith(GfVec3d(point));
            }
        }
        return range;
    }

    // Every mesh the chain ends with.
    std::vector<SdfPath> meshes() const
    {
        std::vector<SdfPath> result;
        for (const SdfPath& path : HdSceneIndexPrimView(end)) {
            if (end->GetPrim(path).primType == HdPrimTypeTokens->mesh) {
                result.push_back(path);
            }
        }
        return result;
    }

    UsdStageRefPtr                      stage;
    UsdImagingStageSceneIndexRefPtr     stageSceneIndex;
    HdVP2FallbackExtentSceneIndexRefPtr fallback;
    HdSceneIndexBaseRefPtr              end;
};

// UsdImaging interpolates the points of a stand-in in single precision, so they span the extent
// to within rounding, and are not equal to it.
bool isClose(const GfRange3d& a, const GfRange3d& b)
{
    return GfIsClose(a.GetMin(), b.GetMin(), 1e-5) && GfIsClose(a.GetMax(), b.GetMax(), 1e-5);
}

const SdfPath   kShape("/Shape");
const SdfPath   kModel("/Model");
const GfRange3d kUnitBox(GfVec3d(-0.5), GfVec3d(0.5));
const GfRange3d kBox(GfVec3d(-1), GfVec3d(1));

// What the schemas declare as the extent of each implicit type.
struct Fallback
{
    const char* type;
    GfRange3d   range;
};
const std::vector<Fallback> kFallbacks {
    { "Cube", kBox },
    { "Sphere", kBox },
    { "Cone", kBox },
    { "Cylinder", kBox },
    { "Capsule", GfRange3d(GfVec3d(-0.5, -0.5, -1), GfVec3d(0.5, 0.5, 1)) },
    { "Plane", GfRange3d(GfVec3d(-1, -1, 0), GfVec3d(1, 1, 0)) },
};

UsdStageRefPtr makeShapeStage(const char* type)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->DefinePrim(kShape, TfToken(type));
    return stage;
}

void setExtent(const UsdStageRefPtr& stage, const SdfPath& path, const GfRange3d& range)
{
    UsdGeomBoundable(stage->GetPrimAtPath(path))
        .CreateExtentAttr(
            VtValue(VtVec3fArray { GfVec3f(range.GetMin()), GfVec3f(range.GetMax()) }));
}

// A cube whose authored extent is the unit box.
void addUnitCube(const UsdStageRefPtr& stage, const SdfPath& path)
{
    UsdGeomCube::Define(stage, path).CreateSizeAttr(VtValue(1.0));
    setExtent(stage, path, kUnitBox);
}

// A model of the given kind that applies the model schema, with the given draw mode authored if
// there is one. Applying the schema is what makes UsdImaging publish the draw mode.
UsdPrim defineModel(
    const UsdStageRefPtr& stage,
    const SdfPath&        path,
    const TfToken&        kind,
    const char*           drawMode)
{
    UsdPrim prim = UsdGeomXform::Define(stage, path).GetPrim();
    UsdModelAPI(prim).SetKind(kind);
    UsdGeomModelAPI api = UsdGeomModelAPI::Apply(prim);
    if (drawMode) {
        api.CreateModelDrawModeAttr(VtValue(TfToken(drawMode)));
    }
    return prim;
}

// A component model drawn with the given draw mode, holding a cube with an authored extent and
// no extent or extentsHint of its own.
UsdStageRefPtr makeModelStage(const char* drawMode)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    defineModel(stage, kModel, KindTokens->component, drawMode);
    addUnitCube(stage, kModel.AppendChild(TfToken("Cube")));
    return stage;
}

const SdfPath kCards = kModel.AppendChild(TfToken("cardsMesh"));
const SdfPath kBounds = kModel.AppendChild(TfToken("boundsCurves"));

} // namespace

TEST(HdVP2FallbackExtent, implicitSurfacesGetTheSchemaFallback)
{
    for (const Fallback& fallback : kFallbacks) {
        Chain chain(makeShapeStage(fallback.type));
        ASSERT_EQ(chain.end->GetPrim(kShape).primType, HdPrimTypeTokens->mesh) << fallback.type;
        EXPECT_EQ(chain.extent(kShape), fallback.range) << fallback.type;
    }
}

TEST(HdVP2FallbackExtent, authoredExtentOfAnImplicitSurfaceWins)
{
    const GfRange3d authored(GfVec3d(-4), GfVec3d(4));
    for (const Fallback& fallback : kFallbacks) {
        UsdStageRefPtr stage = makeShapeStage(fallback.type);
        setExtent(stage, kShape, authored);
        Chain chain(stage);
        EXPECT_EQ(chain.extent(kShape), authored) << fallback.type;
    }
}

TEST(HdVP2FallbackExtent, animatedExtentOfAnImplicitSurfaceIsNotShadowedByTheFallback)
{
    UsdStageRefPtr stage = makeShapeStage("Sphere");
    UsdAttribute   extent = UsdGeomBoundable(stage->GetPrimAtPath(kShape)).CreateExtentAttr();
    extent.Set(VtVec3fArray { GfVec3f(-1.0f), GfVec3f(1.0f) }, UsdTimeCode(0.0));
    extent.Set(VtVec3fArray { GfVec3f(-4.0f), GfVec3f(4.0f) }, UsdTimeCode(10.0));
    Chain chain(stage);

    chain.setTime(10.0);

    EXPECT_EQ(chain.extent(kShape), GfRange3d(GfVec3d(-4), GfVec3d(4)));
}

TEST(HdVP2FallbackExtent, fallbackDoesNotFollowTheSize)
{
    // As UsdImagingDelegate: the attribute is read, and an unauthored one is the schema's.
    UsdStageRefPtr stage = makeShapeStage("Sphere");
    UsdGeomSphere(stage->GetPrimAtPath(kShape)).CreateRadiusAttr(VtValue(4.0));
    Chain chain(stage);
    EXPECT_EQ(chain.extent(kShape), kBox);
}

TEST(HdVP2FallbackExtent, meshesWithoutAnExtentStayWithout)
{
    // The mesh schema declares no fallback, so there is nothing for UsdImagingDelegate to have
    // read either.
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomMesh::Define(stage, kShape);
    Chain chain(stage);
    EXPECT_TRUE(chain.extent(kShape).IsEmpty());
}

TEST(HdVP2FallbackExtent, nativeInstancePrototypeOfAnImplicitSurfaceHasTheFallback)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform::Define(stage, SdfPath("/Proto"));
    const SdfPath source("/Proto/Shape");
    stage->DefinePrim(source, TfToken("Sphere"));
    for (const char* path : { "/InstanceA", "/InstanceB" }) {
        UsdPrim instance = UsdGeomXform::Define(stage, SdfPath(path)).GetPrim();
        instance.GetReferences().AddInternalReference(SdfPath("/Proto"));
        instance.SetInstanceable(true);
    }

    Chain chain(stage);

    // The source prim, and the prototype both instances share: a mesh at some other path.
    const std::vector<SdfPath> meshes = chain.meshes();
    ASSERT_TRUE(std::any_of(meshes.begin(), meshes.end(), [&](const SdfPath& mesh) {
        return mesh != source;
    })) << "no prototype mesh";
    for (const SdfPath& mesh : meshes) {
        EXPECT_EQ(chain.extent(mesh), kBox) << mesh;
    }
}

TEST(HdVP2FallbackExtent, pointInstancerPrototypeOfAnImplicitSurfaceHasTheFallback)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform::Define(stage, SdfPath("/Prototypes"));
    const SdfPath source("/Prototypes/Shape");
    stage->DefinePrim(source, TfToken("Sphere"));

    UsdGeomPointInstancer instancer = UsdGeomPointInstancer::Define(stage, SdfPath("/PI"));
    instancer.CreatePrototypesRel().AddTarget(source);
    instancer.CreateProtoIndicesAttr().Set(VtIntArray { 0, 0 });
    instancer.CreatePositionsAttr().Set(VtVec3fArray { GfVec3f(0.0f), GfVec3f(5.0f, 0.0f, 0.0f) });

    Chain chain(stage);

    // The prototype the instancer draws: a mesh at some other path than the source prim.
    const std::vector<SdfPath> meshes = chain.meshes();
    ASSERT_TRUE(std::any_of(meshes.begin(), meshes.end(), [&](const SdfPath& mesh) {
        return mesh != source;
    })) << "no prototype mesh";
    for (const SdfPath& mesh : meshes) {
        EXPECT_EQ(chain.extent(mesh), kBox) << mesh;
    }
}

TEST(HdVP2FallbackExtent, cardsOfAModelWithoutAnExtentSpanItsChildren)
{
    Chain chain(makeModelStage("cards"));

    ASSERT_EQ(chain.end->GetPrim(kCards).primType, HdPrimTypeTokens->mesh) << "no cards";
    EXPECT_EQ(chain.extent(kCards), kUnitBox);
    EXPECT_TRUE(isClose(chain.pointsRange(kCards), kUnitBox)) << chain.pointsRange(kCards);
}

TEST(HdVP2FallbackExtent, boundsOfAModelWithoutAnExtentAreTheBoxOfItsChildren)
{
    Chain chain(makeModelStage("bounds"));

    ASSERT_EQ(chain.end->GetPrim(kBounds).primType, HdPrimTypeTokens->basisCurves) << "no bounds";
    EXPECT_EQ(chain.extent(kBounds), kUnitBox);
    EXPECT_TRUE(isClose(chain.pointsRange(kBounds), kUnitBox)) << chain.pointsRange(kBounds);
}

TEST(HdVP2FallbackExtent, originOfAModelWithoutAnExtentHasTheBoxOfItsChildren)
{
    // The axes of the origin stand-in do not depend on the extent, which is the one of its prim.
    Chain chain(makeModelStage("origin"));

    const SdfPath origin = kModel.AppendChild(TfToken("originCurves"));
    ASSERT_EQ(chain.end->GetPrim(origin).primType, HdPrimTypeTokens->basisCurves) << "no origin";
    EXPECT_EQ(chain.extent(origin), kUnitBox);
}

TEST(HdVP2FallbackExtent, drawModeInheritedFromAnAncestorIsHonored)
{
    // The ancestor sets the draw mode, the component applies it without authoring its own.
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    defineModel(stage, SdfPath("/World"), KindTokens->group, "cards");
    const SdfPath model("/World/Model");
    defineModel(stage, model, KindTokens->component, nullptr);
    addUnitCube(stage, model.AppendChild(TfToken("Cube")));

    Chain chain(stage);

    const SdfPath cards = model.AppendChild(TfToken("cardsMesh"));
    ASSERT_EQ(chain.end->GetPrim(cards).primType, HdPrimTypeTokens->mesh) << "no cards";
    EXPECT_EQ(chain.extent(cards), kUnitBox);
}

TEST(HdVP2FallbackExtent, extentsHintOfAModelWins)
{
    const GfRange3d hint(GfVec3d(-2), GfVec3d(2));
    UsdStageRefPtr  stage = makeModelStage("cards");
    UsdGeomModelAPI(stage->GetPrimAtPath(kModel))
        .SetExtentsHint(VtVec3fArray { GfVec3f(hint.GetMin()), GfVec3f(hint.GetMax()) });

    Chain chain(stage);

    ASSERT_EQ(chain.end->GetPrim(kCards).primType, HdPrimTypeTokens->mesh) << "no cards";
    EXPECT_EQ(chain.extent(kCards), hint);
}

TEST(HdVP2FallbackExtent, animatedExtentsHintOfAModelFollowsTheTime)
{
    // The hint is resolved by UsdImaging, at the time of the stage; the box of the children is
    // computed once, at the earliest time, so it must not stand in for a hint.
    UsdStageRefPtr  stage = makeModelStage("cards");
    UsdGeomModelAPI api(stage->GetPrimAtPath(kModel));
    api.SetExtentsHint(VtVec3fArray { GfVec3f(-2.0f), GfVec3f(2.0f) }, UsdTimeCode(0.0));
    api.SetExtentsHint(VtVec3fArray { GfVec3f(-3.0f), GfVec3f(3.0f) }, UsdTimeCode(10.0));
    Chain chain(stage);

    chain.setTime(10.0);

    ASSERT_EQ(chain.end->GetPrim(kCards).primType, HdPrimTypeTokens->mesh) << "no cards";
    EXPECT_EQ(chain.extent(kCards), GfRange3d(GfVec3d(-3), GfVec3d(3)));
}

TEST(HdVP2FallbackExtent, modelsWithoutADrawModeGetNoExtent)
{
    {
        UsdStageRefPtr stage = makeModelStage("default");
        Chain          chain(stage);
        EXPECT_TRUE(chain.extent(kModel).IsEmpty());
    }
    {
        // The model schema is applied but no draw mode is authored.
        UsdStageRefPtr stage = makeModelStage(nullptr);
        Chain          chain(stage);
        EXPECT_TRUE(chain.extent(kModel).IsEmpty());
    }
}

TEST(HdVP2FallbackExtent, primsThatAreNotDrawnAsStandInsGetNoExtent)
{
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform::Define(stage, kModel);
        addUnitCube(stage, kModel.AppendChild(TfToken("Cube")));

        Chain chain(stage);

        EXPECT_TRUE(chain.extent(kModel).IsEmpty());
    }
    {
        // A group authors a draw mode for the components below it, and is not drawn with it.
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        defineModel(stage, kModel, KindTokens->group, "cards");
        addUnitCube(stage, kModel.AppendChild(TfToken("Cube")));

        Chain chain(stage);

        EXPECT_TRUE(chain.extent(kModel).IsEmpty());
    }
}

TEST(HdVP2FallbackExtent, drawModeAppliedByAttributeGetsAnExtent)
{
    // Not a component, and made to apply its draw mode by the attribute.
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim        group = defineModel(stage, kModel, KindTokens->group, "cards");
    UsdGeomModelAPI(group).CreateModelApplyDrawModeAttr(VtValue(true));
    addUnitCube(stage, kModel.AppendChild(TfToken("Cube")));

    Chain chain(stage);

    ASSERT_EQ(chain.end->GetPrim(kCards).primType, HdPrimTypeTokens->mesh) << "no cards";
    EXPECT_EQ(chain.extent(kCards), kUnitBox);
}

TEST(HdVP2FallbackExtent, nativeInstanceDrawnAsCardsGetsTheBoxOfItsPrototype)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform::Define(stage, SdfPath("/Proto"));
    addUnitCube(stage, SdfPath("/Proto/Cube"));
    const SdfPath instance("/Instance");
    UsdPrim       prim = defineModel(stage, instance, KindTokens->component, "cards");
    prim.GetReferences().AddInternalReference(SdfPath("/Proto"));
    prim.SetInstanceable(true);

    Chain chain(stage);

    // The cards of an instance are drawn by the instancer it is aggregated into.
    std::vector<SdfPath> cards;
    for (const SdfPath& path : chain.meshes()) {
        if (path.GetName() == "cardsMesh") {
            cards.push_back(path);
        }
    }
    ASSERT_FALSE(cards.empty()) << "no cards";
    for (const SdfPath& path : cards) {
        EXPECT_EQ(chain.extent(path), kUnitBox) << path;
    }
}
