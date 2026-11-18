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
#include <mayaUsd/render/vp2RenderDelegate/sceneIndex/implicitDoubleSidedSceneIndex.h>

#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/sceneIndexPrimView.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdsi/implicitSurfaceSceneIndex.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <set>
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

// The paths whose doubleSided was dirtied, as the render index would be told.
class DoubleSidedRecorder : public HdSceneIndexObserver
{
public:
    std::set<SdfPath> dirtied;

    void PrimsAdded(const HdSceneIndexBase&, const AddedPrimEntries&) override { }
    void PrimsRemoved(const HdSceneIndexBase&, const RemovedPrimEntries&) override { }
    void PrimsDirtied(const HdSceneIndexBase&, const DirtiedPrimEntries& entries) override
    {
        for (const DirtiedPrimEntry& entry : entries) {
            if (entry.dirtyLocators.Intersects(HdMeshSchema::GetDoubleSidedLocator())) {
                dirtied.insert(entry.primPath);
            }
        }
    }
    void PrimsRenamed(const HdSceneIndexBase&, const RenamedPrimEntries&) override { }
};

// The producer's chain from the stage scene index to the restored meshes: the tagging scene index
// where the producer installs it, ahead of instancing, then the implicit surface conversion, then
// the restoring scene index.
struct Chain
{
    explicit Chain(const UsdStageRefPtr& stage_)
        : stage(stage_)
    {
        UsdImagingCreateSceneIndicesInfo info;
        info.overridesSceneIndexCallback
            = [this](const HdSceneIndexBaseRefPtr& input) -> HdSceneIndexBaseRefPtr {
            tagging = HdVP2DoubleSidedTaggingSceneIndex::New(input);
            return tagging;
        };

        const UsdImagingSceneIndices indices = UsdImagingCreateSceneIndices(info);
        stageSceneIndex = indices.stageSceneIndex;
        end = HdVP2DoubleSidedRestoringSceneIndex::New(
            HdsiImplicitSurfaceSceneIndex::New(indices.finalSceneIndex, implicitSurfaceArgs()));
        end->AddObserver(HdSceneIndexObserverPtr(&recorder));

        // In the producer's order: the tagging scene index first, so the first prim has its tag.
        tagging->SetStage(stage);
        stageSceneIndex->SetStage(stage);
    }

    // As HdVP2SceneIndexProducer::ApplyPendingUpdates does.
    void applyEdits()
    {
        stageSceneIndex->ApplyPendingUpdates();
        tagging->ApplyPendingUpdates();
    }

    // The doubleSided the render index would read, or nothing if the prim is not a mesh.
    std::optional<bool> doubleSided(const SdfPath& path) const
    {
        const HdSceneIndexPrim prim = end->GetPrim(path);
        if (prim.primType != HdPrimTypeTokens->mesh) {
            return std::nullopt;
        }
        const HdBoolDataSourceHandle value
            = HdMeshSchema::GetFromParent(prim.dataSource).GetDoubleSided();
        if (!value) {
            return std::nullopt;
        }
        return value->GetTypedValue(0.0f);
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

    UsdStageRefPtr                          stage;
    UsdImagingStageSceneIndexRefPtr         stageSceneIndex;
    HdVP2DoubleSidedTaggingSceneIndexRefPtr tagging;
    HdSceneIndexBaseRefPtr                  end;
    DoubleSidedRecorder                     recorder;
};

const SdfPath kShape("/Shape");

// A prim of the given implicit type at /Shape; the attribute is authored only if a value is given.
UsdStageRefPtr makeShapeStage(const char* type, std::optional<bool> doubleSided)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim        prim = stage->DefinePrim(kShape, TfToken(type));
    if (doubleSided) {
        UsdGeomGprim(prim).CreateDoubleSidedAttr(VtValue(*doubleSided));
    }
    return stage;
}

void setDoubleSided(const UsdStageRefPtr& stage, const SdfPath& path, bool value)
{
    UsdGeomGprim(stage->GetPrimAtPath(path)).CreateDoubleSidedAttr(VtValue(value));
}

const std::vector<const char*> kImplicitTypes { "Cube", "Sphere", "Cone", "Cylinder", "Capsule" };

} // namespace

TEST(HdVP2DoubleSidedSceneIndex, implicitSurfacesKeepAuthoredDoubleSided)
{
    for (const char* type : kImplicitTypes) {
        Chain chain(makeShapeStage(type, true));
        ASSERT_TRUE(chain.doubleSided(kShape).has_value()) << type << " did not become a mesh";
        EXPECT_TRUE(*chain.doubleSided(kShape)) << type;
    }
}

TEST(HdVP2DoubleSidedSceneIndex, implicitSurfacesAuthoredFalseStayFalse)
{
    for (const char* type : kImplicitTypes) {
        Chain chain(makeShapeStage(type, false));
        ASSERT_TRUE(chain.doubleSided(kShape).has_value()) << type << " did not become a mesh";
        EXPECT_FALSE(*chain.doubleSided(kShape)) << type;
    }
}

TEST(HdVP2DoubleSidedSceneIndex, implicitSurfacesWithoutTheAttributeAreSingleSided)
{
    for (const char* type : kImplicitTypes) {
        Chain chain(makeShapeStage(type, std::nullopt));
        ASSERT_TRUE(chain.doubleSided(kShape).has_value()) << type << " did not become a mesh";
        EXPECT_FALSE(*chain.doubleSided(kShape)) << type;
    }
}

TEST(HdVP2DoubleSidedSceneIndex, meshesAreLeftAlone)
{
    for (const bool authored : { true, false }) {
        Chain chain(makeShapeStage("Mesh", authored));
        ASSERT_TRUE(chain.doubleSided(kShape).has_value());
        EXPECT_EQ(*chain.doubleSided(kShape), authored);
    }
}

TEST(HdVP2DoubleSidedSceneIndex, editedValueIsDirtiedAndRead)
{
    Chain chain(makeShapeStage("Cube", true));
    ASSERT_TRUE(chain.doubleSided(kShape).value_or(false));

    setDoubleSided(chain.stage, kShape, false);
    chain.applyEdits();

    EXPECT_EQ(chain.recorder.dirtied.count(kShape), 1u);
    EXPECT_FALSE(chain.doubleSided(kShape).value_or(true));
}

TEST(HdVP2DoubleSidedSceneIndex, newlyAuthoredValueIsDirtiedAndRead)
{
    // No attribute spec yet, so the edit arrives as a resync of the property, not as a change.
    Chain chain(makeShapeStage("Cube", std::nullopt));
    ASSERT_FALSE(chain.doubleSided(kShape).value_or(true));

    setDoubleSided(chain.stage, kShape, true);
    chain.applyEdits();

    EXPECT_EQ(chain.recorder.dirtied.count(kShape), 1u);
    EXPECT_TRUE(chain.doubleSided(kShape).value_or(false));
}

TEST(HdVP2DoubleSidedSceneIndex, planesKeepTheirOwnDoubleSided)
{
    // HdPlaneSchema carries doubleSided, so the conversion copies it and nothing is restored.
    for (const bool authored : { true, false }) {
        Chain chain(makeShapeStage("Plane", authored));
        ASSERT_TRUE(chain.doubleSided(kShape).has_value());
        EXPECT_EQ(*chain.doubleSided(kShape), authored);
    }
}

TEST(HdVP2DoubleSidedSceneIndex, otherAttributesAreNotDirtied)
{
    // The size is authored up front, so that changing it is a change of value and not a new spec.
    UsdStageRefPtr stage = makeShapeStage("Cube", true);
    UsdGeomCube    cube(stage->GetPrimAtPath(kShape));
    cube.CreateSizeAttr(VtValue(2.0));
    Chain chain(stage);

    cube.GetSizeAttr().Set(4.0);
    chain.applyEdits();

    EXPECT_TRUE(chain.recorder.dirtied.empty());
}

TEST(HdVP2DoubleSidedSceneIndex, nativeInstancePrototypeKeepsAndFollowsTheValue)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform::Define(stage, SdfPath("/Proto"));
    const SdfPath source("/Proto/Shape");
    UsdPrim       shape = stage->DefinePrim(source, TfToken("Cube"));
    UsdGeomGprim(shape).CreateDoubleSidedAttr(VtValue(true));
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
        EXPECT_TRUE(chain.doubleSided(mesh).value_or(false)) << mesh;
    }

    setDoubleSided(stage, source, false);
    chain.applyEdits();

    for (const SdfPath& mesh : meshes) {
        EXPECT_EQ(chain.recorder.dirtied.count(mesh), 1u) << mesh;
        EXPECT_FALSE(chain.doubleSided(mesh).value_or(true)) << mesh;
    }
}

TEST(HdVP2DoubleSidedSceneIndex, pointInstancerPrototypeKeepsAndFollowsTheValue)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform::Define(stage, SdfPath("/Prototypes"));
    const SdfPath source("/Prototypes/Shape");
    UsdPrim       shape = stage->DefinePrim(source, TfToken("Cube"));
    UsdGeomGprim(shape).CreateDoubleSidedAttr(VtValue(true));

    UsdGeomPointInstancer instancer = UsdGeomPointInstancer::Define(stage, SdfPath("/PI"));
    instancer.CreatePrototypesRel().AddTarget(source);
    instancer.CreateProtoIndicesAttr().Set(VtIntArray { 0, 0 });
    instancer.CreatePositionsAttr().Set(VtVec3fArray { GfVec3f(0.0f), GfVec3f(5.0f, 0.0f, 0.0f) });

    Chain chain(stage);

    // The prototype the instancer draws: a mesh at some other path than the source prim, which
    // may or may not draw on its own.
    const std::vector<SdfPath> meshes = chain.meshes();
    ASSERT_TRUE(std::any_of(meshes.begin(), meshes.end(), [&](const SdfPath& mesh) {
        return mesh != source;
    })) << "no prototype mesh";
    for (const SdfPath& mesh : meshes) {
        EXPECT_TRUE(chain.doubleSided(mesh).value_or(false)) << mesh;
    }

    setDoubleSided(stage, source, false);
    chain.applyEdits();

    for (const SdfPath& mesh : meshes) {
        EXPECT_EQ(chain.recorder.dirtied.count(mesh), 1u) << mesh;
        EXPECT_FALSE(chain.doubleSided(mesh).value_or(true)) << mesh;
    }
}
