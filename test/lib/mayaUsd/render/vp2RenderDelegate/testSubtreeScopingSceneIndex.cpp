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
#include <mayaUsd/render/vp2RenderDelegate/sceneIndex/subtreeScopingSceneIndex.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/imaging/hd/cubeSchema.h>
#include <pxr/imaging/hd/instancerTopologySchema.h>
#include <pxr/imaging/hd/materialBindingsSchema.h>
#include <pxr/imaging/hd/sceneIndexPrimView.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdsi/materialBindingResolvingSceneIndex.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/collectionAPI.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>

#include <gtest/gtest.h>

#include <map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

enum class Binding
{
    Collection,
    Direct,
    None
};

const SdfPath kCube("/Asset/Geo/Cube");
const SdfPath kMaterial("/Asset/Looks/Red");

// Authors /Asset/Geo/Cube with a red material in /Asset/Looks, outside the
// /Asset/Geo scope, bound as requested.
UsdStageRefPtr makeStage(Binding binding)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    UsdGeomXform::Define(stage, SdfPath("/Asset"));
    UsdGeomXform::Define(stage, SdfPath("/Asset/Geo"));
    UsdGeomCube::Define(stage, kCube);
    UsdGeomScope::Define(stage, SdfPath("/Asset/Looks"));
    UsdShadeMaterial material = UsdShadeMaterial::Define(stage, kMaterial);
    UsdShadeShader   shader = UsdShadeShader::Define(stage, SdfPath("/Asset/Looks/Red/Surface"));
    shader.CreateIdAttr().Set(TfToken("UsdPreviewSurface"));
    shader.CreateInput(TfToken("diffuseColor"), SdfValueTypeNames->Color3f)
        .Set(GfVec3f(1.0f, 0.0f, 0.0f));
    material.CreateSurfaceOutput().ConnectToSource(shader.ConnectableAPI(), TfToken("surface"));

    if (binding == Binding::Collection) {
        UsdCollectionAPI collection = UsdCollectionAPI::Apply(
            stage->GetPrimAtPath(SdfPath("/Asset/Looks")), TfToken("redGeo"));
        collection.CreateIncludesRel().AddTarget(kCube);
        UsdShadeMaterialBindingAPI::Apply(stage->GetPrimAtPath(SdfPath("/Asset")))
            .Bind(collection, material, TfToken("redGeo"));
    } else if (binding == Binding::Direct) {
        UsdShadeMaterialBindingAPI::Apply(stage->GetPrimAtPath(kCube)).Bind(material);
    }
    return stage;
}

// Builds the producer's chain up to the binding resolver: the stage scene index
// with scoping installed as the producer does, then material binding resolution.
HdSceneIndexBaseRefPtr makeChain(
    const UsdStageRefPtr&                stage,
    const SdfPath&                       root,
    HdVP2SubtreeScopingSceneIndexRefPtr& scoping,
    UsdImagingStageSceneIndexRefPtr*     stageSceneIndex = nullptr)
{
    UsdImagingCreateSceneIndicesInfo info;
    info.overridesSceneIndexCallback
        = [&](const HdSceneIndexBaseRefPtr& input) -> HdSceneIndexBaseRefPtr {
        scoping = HdVP2SubtreeScopingSceneIndex::New(input);
        scoping->SetRootPath(root);
        return scoping;
    };
    info.stage = stage;

    const UsdImagingSceneIndices indices = UsdImagingCreateSceneIndices(info);
    if (stageSceneIndex) {
        *stageSceneIndex = indices.stageSceneIndex;
    }
    return HdsiMaterialBindingResolvingSceneIndex::New(
        indices.finalSceneIndex,
        { HdMaterialBindingsSchemaTokens->allPurpose },
        HdMaterialBindingsSchemaTokens->allPurpose);
}

// The material path the cube resolves to, or an empty path if it has none.
SdfPath resolvedMaterial(Binding binding, const SdfPath& root)
{
    HdVP2SubtreeScopingSceneIndexRefPtr scoping;
    HdSceneIndexBaseRefPtr              chain = makeChain(makeStage(binding), root, scoping);

    HdSceneIndexPrim       prim = chain->GetPrim(kCube);
    HdPathDataSourceHandle path
        = HdMaterialBindingsSchema::GetFromParent(prim.dataSource).GetMaterialBinding().GetPath();
    return path ? path->GetTypedValue(0.0f) : SdfPath();
}

// Authors /Asset/Geo/Cube and /Other/Cube, plus native instances of a prototype
// at /Asset/Geo/Inst (in scope) and /Other/Inst (outside it).
UsdStageRefPtr makeInstancedStage(bool instanceInScope, bool instanceOutside)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    UsdGeomXform::Define(stage, SdfPath("/Asset"));
    UsdGeomXform::Define(stage, SdfPath("/Asset/Geo"));
    UsdGeomCube::Define(stage, kCube);
    UsdGeomXform::Define(stage, SdfPath("/Other"));
    UsdGeomCube::Define(stage, SdfPath("/Other/Cube"));
    UsdGeomXform::Define(stage, SdfPath("/Proto"));
    UsdGeomCube::Define(stage, SdfPath("/Proto/Cube"));

    auto instance = [&](const char* path) {
        UsdPrim prim = UsdGeomXform::Define(stage, SdfPath(path)).GetPrim();
        prim.GetReferences().AddInternalReference(SdfPath("/Proto"));
        prim.SetInstanceable(true);
    };
    if (instanceInScope) {
        instance("/Asset/Geo/Inst");
    }
    if (instanceOutside) {
        instance("/Other/Inst");
    }
    return stage;
}

// The number of instances aggregated into instancers, summed over all of them.
size_t instanceCount(const HdSceneIndexBaseRefPtr& chain)
{
    size_t count = 0;
    for (const SdfPath& path : HdSceneIndexPrimView(chain)) {
        const HdSceneIndexPrim prim = chain->GetPrim(path);
        if (prim.primType != HdPrimTypeTokens->instancer) {
            continue;
        }
        HdIntArrayVectorSchema indices
            = HdInstancerTopologySchema::GetFromParent(prim.dataSource).GetInstanceIndices();
        for (size_t i = 0; i < indices.GetNumElements(); ++i) {
            if (HdIntArrayDataSourceHandle element = indices.GetElement(i)) {
                count += element->GetTypedValue(0.0f).size();
            }
        }
    }
    return count;
}

size_t instanceCount(bool instanceInScope, bool instanceOutside, const SdfPath& root)
{
    HdVP2SubtreeScopingSceneIndexRefPtr scoping;
    return instanceCount(
        makeChain(makeInstancedStage(instanceInScope, instanceOutside), root, scoping));
}

const SdfPath kInstancer("/World/Instances/PI");
const SdfPath kProtoCube("/World/Prototypes/Cube");
const SdfPath kProtoUnused("/World/Prototypes/Unused");

void definePointInstancer(const UsdStageRefPtr& stage)
{
    UsdGeomPointInstancer instancer = UsdGeomPointInstancer::Define(stage, kInstancer);
    instancer.CreatePrototypesRel().AddTarget(kProtoCube);
    instancer.CreateProtoIndicesAttr().Set(VtIntArray { 0, 0 });
    instancer.CreatePositionsAttr().Set(VtVec3fArray { GfVec3f(0.0f), GfVec3f(5.0f, 0.0f, 0.0f) });
}

// Authors a point instancer at /World/Instances/PI whose prototype is
// /World/Prototypes/Cube, outside /World/Instances. A second cube under
// /World/Prototypes, which no instancer references, stands for the rest of the
// outside content.
UsdStageRefPtr makePointInstancerStage(bool withInstancer = true)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    UsdGeomXform::Define(stage, SdfPath("/World"));
    UsdGeomXform::Define(stage, SdfPath("/World/Prototypes"));
    UsdGeomCube::Define(stage, kProtoCube);
    UsdGeomCube::Define(stage, kProtoUnused);
    UsdGeomXform::Define(stage, SdfPath("/World/Instances"));

    if (withInstancer) {
        definePointInstancer(stage);
    }
    return stage;
}

// Records the prim type the scoping index last announced for each path, to
// check what its notices say, not only what GetPrim returns.
class TypeRecorder : public HdSceneIndexObserver
{
public:
    std::map<SdfPath, TfToken> types;

    void PrimsAdded(const HdSceneIndexBase&, const AddedPrimEntries& entries) override
    {
        for (const AddedPrimEntry& entry : entries) {
            types[entry.primPath] = entry.primType;
        }
    }
    void PrimsRemoved(const HdSceneIndexBase&, const RemovedPrimEntries& entries) override
    {
        for (const RemovedPrimEntry& entry : entries) {
            for (auto it = types.begin(); it != types.end();) {
                it = it->first.HasPrefix(entry.primPath) ? types.erase(it) : std::next(it);
            }
        }
    }
    void PrimsDirtied(const HdSceneIndexBase&, const DirtiedPrimEntries&) override { }
    void PrimsRenamed(const HdSceneIndexBase&, const RenamedPrimEntries&) override { }
};

// The point instancer stage scoped to /World/Instances, which a test edits and
// then flushes to the scoping index.
struct ScopedPointInstancer
{
    explicit ScopedPointInstancer(bool withInstancer)
        : stage(makePointInstancerStage(withInstancer))
        , chain(makeChain(stage, SdfPath("/World/Instances"), scoping, &stageSceneIndex))
    {
        scoping->AddObserver(HdSceneIndexObserverPtr(&recorder));
    }

    void applyEdits() { stageSceneIndex->ApplyPendingUpdates(); }

    TfToken type(const SdfPath& path) const { return scoping->GetPrim(path).primType; }

    UsdStageRefPtr                      stage;
    HdVP2SubtreeScopingSceneIndexRefPtr scoping;
    UsdImagingStageSceneIndexRefPtr     stageSceneIndex;
    HdSceneIndexBaseRefPtr              chain;
    TypeRecorder                        recorder;
};

// The prim type of every prototype the chain's instancers draw from, keyed by
// the path the instancer names it with.
std::map<SdfPath, TfToken> prototypeTypes(const HdSceneIndexBaseRefPtr& chain)
{
    std::map<SdfPath, TfToken> types;
    for (const SdfPath& path : HdSceneIndexPrimView(chain)) {
        const HdSceneIndexPrim prim = chain->GetPrim(path);
        if (prim.primType != HdPrimTypeTokens->instancer) {
            continue;
        }
        HdPathArrayDataSourceHandle prototypes
            = HdInstancerTopologySchema::GetFromParent(prim.dataSource).GetPrototypes();
        if (!prototypes) {
            continue;
        }
        for (const SdfPath& prototype : prototypes->GetTypedValue(0.0f)) {
            types[prototype] = chain->GetPrim(prototype).primType;
        }
    }
    return types;
}

std::map<SdfPath, TfToken> prototypeTypes(const SdfPath& root)
{
    HdVP2SubtreeScopingSceneIndexRefPtr scoping;
    return prototypeTypes(makeChain(makePointInstancerStage(), root, scoping));
}

} // namespace

TEST(HdVP2SubtreeScopingSceneIndex, collectionBindingOutsideScopeResolves)
{
    EXPECT_EQ(resolvedMaterial(Binding::Collection, SdfPath("/Asset/Geo")), kMaterial);
}

TEST(HdVP2SubtreeScopingSceneIndex, directBindingOutsideScopeResolves)
{
    EXPECT_EQ(resolvedMaterial(Binding::Direct, SdfPath("/Asset/Geo")), kMaterial);
}

TEST(HdVP2SubtreeScopingSceneIndex, noBindingResolvesToNothing)
{
    EXPECT_TRUE(resolvedMaterial(Binding::None, SdfPath("/Asset/Geo")).IsEmpty());
}

TEST(HdVP2SubtreeScopingSceneIndex, unscopedCollectionBindingResolves)
{
    EXPECT_EQ(resolvedMaterial(Binding::Collection, SdfPath::AbsoluteRootPath()), kMaterial);
}

TEST(HdVP2SubtreeScopingSceneIndex, outsidePrimKeepsDataButNotType)
{
    HdVP2SubtreeScopingSceneIndexRefPtr scoping;
    HdSceneIndexBaseRefPtr              chain
        = makeChain(makeStage(Binding::None), SdfPath("/Asset/Looks"), scoping);

    // Any schema is kept, not only the ones something is known to read.
    const HdSceneIndexPrim prim = chain->GetPrim(kCube);
    EXPECT_TRUE(prim.primType.IsEmpty());
    EXPECT_TRUE(HdCubeSchema::GetFromParent(prim.dataSource).IsDefined());
}

TEST(HdVP2SubtreeScopingSceneIndex, instanceInScopeIsAggregated)
{
    EXPECT_EQ(instanceCount(true, false, SdfPath("/Asset/Geo")), 1u);
}

TEST(HdVP2SubtreeScopingSceneIndex, instanceOutsideScopeIsNotAggregated)
{
    EXPECT_EQ(instanceCount(false, true, SdfPath("/Asset/Geo")), 0u);
}

TEST(HdVP2SubtreeScopingSceneIndex, instanceOutsideScopeDoesNotJoinInScopeInstancer)
{
    EXPECT_EQ(instanceCount(true, true, SdfPath("/Asset/Geo")), 1u);
}

TEST(HdVP2SubtreeScopingSceneIndex, unscopedInstancesAreAggregated)
{
    EXPECT_EQ(instanceCount(true, true, SdfPath::AbsoluteRootPath()), 2u);
}

TEST(HdVP2SubtreeScopingSceneIndex, unscopedPointInstancerPrototypeHasType)
{
    // Control: without scoping the prototype keeps its type, which is what the
    // scoped case below must match.
    const std::map<SdfPath, TfToken> types = prototypeTypes(SdfPath::AbsoluteRootPath());
    ASSERT_EQ(types.size(), 1u);
    EXPECT_EQ(types.begin()->second, HdPrimTypeTokens->cube);
}

TEST(HdVP2SubtreeScopingSceneIndex, pointInstancerPrototypeOutsideScopeKeepsType)
{
    const std::map<SdfPath, TfToken> types = prototypeTypes(SdfPath("/World/Instances"));
    ASSERT_EQ(types.size(), 1u);
    EXPECT_EQ(types.begin()->second, HdPrimTypeTokens->cube);
}

TEST(HdVP2SubtreeScopingSceneIndex, unreferencedPrototypeOutsideScopeHasNoType)
{
    ScopedPointInstancer scene(true);

    EXPECT_TRUE(scene.type(kProtoUnused).IsEmpty());
}

TEST(HdVP2SubtreeScopingSceneIndex, onlyTheNamedPrototypeSubtreeIsKept)
{
    ScopedPointInstancer scene(true);

    EXPECT_EQ(scene.type(kProtoCube), HdPrimTypeTokens->cube);
    EXPECT_TRUE(scene.type(SdfPath("/World/Prototypes")).IsEmpty());
}

TEST(HdVP2SubtreeScopingSceneIndex, instancerAddedLaterGivesItsPrototypeAType)
{
    ScopedPointInstancer scene(false);
    EXPECT_TRUE(scene.type(kProtoCube).IsEmpty());

    definePointInstancer(scene.stage);
    scene.applyEdits();

    EXPECT_EQ(scene.type(kProtoCube), HdPrimTypeTokens->cube);
    ASSERT_TRUE(scene.recorder.types.count(kProtoCube));
    EXPECT_EQ(scene.recorder.types[kProtoCube], HdPrimTypeTokens->cube);
}

TEST(HdVP2SubtreeScopingSceneIndex, retargetedPrototypeMovesTheKeptSubtree)
{
    ScopedPointInstancer scene(true);

    UsdGeomPointInstancer::Get(scene.stage, kInstancer)
        .GetPrototypesRel()
        .SetTargets({ kProtoUnused });
    scene.applyEdits();

    EXPECT_TRUE(scene.type(kProtoCube).IsEmpty());
    EXPECT_EQ(scene.type(kProtoUnused), HdPrimTypeTokens->cube);
    ASSERT_TRUE(scene.recorder.types.count(kProtoCube));
    EXPECT_TRUE(scene.recorder.types[kProtoCube].IsEmpty());
    ASSERT_TRUE(scene.recorder.types.count(kProtoUnused));
    EXPECT_EQ(scene.recorder.types[kProtoUnused], HdPrimTypeTokens->cube);
}

TEST(HdVP2SubtreeScopingSceneIndex, removedInstancerReleasesItsPrototype)
{
    ScopedPointInstancer scene(true);

    scene.stage->RemovePrim(kInstancer);
    scene.applyEdits();

    EXPECT_TRUE(scene.type(kProtoCube).IsEmpty());
    ASSERT_TRUE(scene.recorder.types.count(kProtoCube));
    EXPECT_TRUE(scene.recorder.types[kProtoCube].IsEmpty());
}
