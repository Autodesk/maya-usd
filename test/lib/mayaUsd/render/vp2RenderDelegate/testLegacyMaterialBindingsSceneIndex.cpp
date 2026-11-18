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
#include <mayaUsd/render/vp2RenderDelegate/sceneIndex/legacyMaterialBindingsSceneIndex.h>

#include <pxr/imaging/hd/materialBindingSchema.h>
#include <pxr/imaging/hd/materialBindingsSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/sceneIndexPrimView.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdsi/materialBindingResolvingSceneIndex.h>
#include <pxr/imaging/hdsi/unboundMaterialPruningSceneIndex.h>
#include <pxr/usd/usd/collectionAPI.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/tokens.h>
#include <pxr/usdImaging/usdImaging/materialBindingsSchema.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <set>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// The material binding purposes VP2 renders, in priority order, as the producer configures them.
const TfTokenVector kPurposes { HdTokens->preview, HdMaterialBindingsSchemaTokens->allPurpose };

// How often the Hydra material bindings of a path were dirtied, as the render index would be told.
class BindingsRecorder : public HdSceneIndexObserver
{
public:
    std::map<SdfPath, int> dirtied;

    void PrimsAdded(const HdSceneIndexBase&, const AddedPrimEntries&) override { }
    void PrimsRemoved(const HdSceneIndexBase&, const RemovedPrimEntries&) override { }
    void PrimsDirtied(const HdSceneIndexBase&, const DirtiedPrimEntries& entries) override
    {
        for (const DirtiedPrimEntry& entry : entries) {
            if (entry.dirtyLocators.Intersects(HdMaterialBindingsSchema::GetDefaultLocator())) {
                ++dirtied[entry.primPath];
            }
        }
    }
    void PrimsRenamed(const HdSceneIndexBase&, const RenamedPrimEntries&) override { }
};

// The producer's chain from the stage scene index to what the render index is given: the legacy
// bindings scene index where the producer installs it, ahead of instancing, then the binding
// resolution and the pruning of unbound materials.
struct Chain
{
    explicit Chain(const UsdStageRefPtr& stage_)
        : stage(stage_)
    {
        UsdImagingCreateSceneIndicesInfo info;
        info.overridesSceneIndexCallback
            = [this](const HdSceneIndexBaseRefPtr& input) -> HdSceneIndexBaseRefPtr {
            legacy = HdVP2LegacyMaterialBindingsSceneIndex::New(input);
            return legacy;
        };

        const UsdImagingSceneIndices indices = UsdImagingCreateSceneIndices(info);
        stageSceneIndex = indices.stageSceneIndex;

        const HdSceneIndexBaseRefPtr resolved = HdsiMaterialBindingResolvingSceneIndex::New(
            indices.finalSceneIndex, kPurposes, HdMaterialBindingsSchemaTokens->allPurpose);
        end = HdsiUnboundMaterialPruningSceneIndex::New(
            resolved,
            HdRetainedContainerDataSource::New(
                HdsiUnboundMaterialPruningSceneIndexTokens->materialBindingPurposes,
                HdRetainedTypedSampledDataSource<VtArray<TfToken>>::New(
                    VtArray<TfToken>(kPurposes.begin(), kPurposes.end()))));
        end->AddObserver(HdSceneIndexObserverPtr(&recorder));

        // In the producer's order: the legacy bindings scene index first, so the first prim has
        // its bindings.
        legacy->SetStage(stage);
        stageSceneIndex->SetStage(stage);
    }

    // As HdVP2SceneIndexProducer::ApplyPendingUpdates does.
    void applyEdits()
    {
        stageSceneIndex->ApplyPendingUpdates();
        legacy->ApplyPendingUpdates();
    }

    // The material the render index would resolve for the prim, or the empty path.
    SdfPath boundMaterial(const SdfPath& path) const
    {
        const HdSceneIndexPrim       prim = end->GetPrim(path);
        const HdPathDataSourceHandle value
            = HdMaterialBindingsSchema::GetFromParent(prim.dataSource)
                  .GetMaterialBinding(HdTokens->preview)
                  .GetPath();
        return value ? value->GetTypedValue(0.0f) : SdfPath();
    }

    // Whether the material is still a material after the unbound ones were pruned.
    bool materialSurvives(const SdfPath& path) const
    {
        return end->GetPrim(path).primType == HdPrimTypeTokens->material;
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

    // What the legacy bindings scene index itself publishes for the prim.
    HdDataSourceBaseHandle ownBindings(const SdfPath& path) const
    {
        const HdSceneIndexPrim prim = legacy->GetPrim(path);
        return prim.dataSource
            ? prim.dataSource->Get(UsdImagingMaterialBindingsSchema::GetSchemaToken())
            : nullptr;
    }

    UsdStageRefPtr                              stage;
    UsdImagingStageSceneIndexRefPtr             stageSceneIndex;
    HdVP2LegacyMaterialBindingsSceneIndexRefPtr legacy;
    HdSceneIndexBaseRefPtr                      end;
    BindingsRecorder                            recorder;
};

const SdfPath kMatA("/Looks/MatA");
const SdfPath kMatB("/Looks/MatB");
const SdfPath kMesh("/Mesh");

// Two materials, as UsdShade would write them.
UsdStageRefPtr makeStage()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomScope::Define(stage, SdfPath("/Looks"));
    UsdShadeMaterial::Define(stage, kMatA);
    UsdShadeMaterial::Define(stage, kMatB);
    return stage;
}

// What a stage written before MaterialBindingAPI was an applied schema holds: the relationship,
// and no apiSchemas.
UsdRelationship bindLegacy(
    const UsdStageRefPtr& stage,
    const SdfPath&        primPath,
    const SdfPath&        material,
    const char*           relName = "material:binding")
{
    UsdRelationship rel
        = stage->GetPrimAtPath(primPath).CreateRelationship(TfToken(relName), /* custom */ false);
    rel.SetTargets({ material });
    return rel;
}

} // namespace

TEST(HdVP2LegacyMaterialBindings, bindingOnAPrimWithoutTheSchemaIsResolved)
{
    UsdStageRefPtr stage = makeStage();
    UsdGeomMesh::Define(stage, kMesh);
    bindLegacy(stage, kMesh, kMatA);

    Chain chain(stage);

    EXPECT_EQ(chain.boundMaterial(kMesh), kMatA);
    EXPECT_TRUE(chain.materialSurvives(kMatA)) << "the only binding of the material was dropped";
    EXPECT_FALSE(chain.materialSurvives(kMatB));
}

TEST(HdVP2LegacyMaterialBindings, bindingOnAnAncestorWithoutTheSchemaIsInherited)
{
    UsdStageRefPtr stage = makeStage();
    UsdGeomXform::Define(stage, SdfPath("/Group"));
    const SdfPath child("/Group/Mesh");
    UsdGeomMesh::Define(stage, child);
    bindLegacy(stage, SdfPath("/Group"), kMatA);

    Chain chain(stage);

    EXPECT_EQ(chain.boundMaterial(child), kMatA);
}

TEST(HdVP2LegacyMaterialBindings, bindingStrengthIsHonored)
{
    for (const bool stronger : { false, true }) {
        UsdStageRefPtr stage = makeStage();
        UsdGeomXform::Define(stage, SdfPath("/Group"));
        const SdfPath child("/Group/Mesh");
        UsdGeomMesh::Define(stage, child);
        UsdRelationship groupRel = bindLegacy(stage, SdfPath("/Group"), kMatA);
        if (stronger) {
            UsdShadeMaterialBindingAPI::SetMaterialBindingStrength(
                groupRel, UsdShadeTokens->strongerThanDescendants);
        }
        bindLegacy(stage, child, kMatB);

        Chain chain(stage);

        // The descendant's own binding wins, unless the ancestor's is stronger than descendants.
        EXPECT_EQ(chain.boundMaterial(child), stronger ? kMatA : kMatB) << stronger;
    }
}

TEST(HdVP2LegacyMaterialBindings, purposeSpecificBindingsAreHonored)
{
    {
        // preview is the purpose VP2 renders, so it wins over allPurpose.
        UsdStageRefPtr stage = makeStage();
        UsdGeomMesh::Define(stage, kMesh);
        bindLegacy(stage, kMesh, kMatA);
        bindLegacy(stage, kMesh, kMatB, "material:binding:preview");

        Chain chain(stage);

        EXPECT_EQ(chain.boundMaterial(kMesh), kMatB);
    }
    {
        // full is not rendered, so it binds nothing.
        UsdStageRefPtr stage = makeStage();
        UsdGeomMesh::Define(stage, kMesh);
        bindLegacy(stage, kMesh, kMatA, "material:binding:full");

        Chain chain(stage);

        EXPECT_TRUE(chain.boundMaterial(kMesh).IsEmpty());
        EXPECT_FALSE(chain.materialSurvives(kMatA));
    }
}

TEST(HdVP2LegacyMaterialBindings, collectionBindingOnAPrimWithoutTheSchemaIsResolved)
{
    // The prim binds by collection without MaterialBindingAPI; the collection applies
    // CollectionAPI, as it has to for UsdImaging to publish it.
    UsdStageRefPtr stage = makeStage();
    const SdfPath  group("/Group");
    UsdGeomXform::Define(stage, group);
    const SdfPath inside("/Group/Inside");
    const SdfPath outside("/Group/Outside");
    UsdGeomMesh::Define(stage, inside);
    UsdGeomMesh::Define(stage, outside);

    UsdCollectionAPI::Apply(stage->GetPrimAtPath(group), TfToken("shiny"))
        .CreateIncludesRel()
        .AddTarget(inside);
    UsdRelationship rel = stage->GetPrimAtPath(group).CreateRelationship(
        TfToken("material:binding:collection:shiny"), /* custom */ false);
    rel.SetTargets({ group.AppendProperty(TfToken("collection:shiny")), kMatA });

    Chain chain(stage);

    EXPECT_EQ(chain.boundMaterial(inside), kMatA);
    EXPECT_TRUE(chain.boundMaterial(outside).IsEmpty());
}

TEST(HdVP2LegacyMaterialBindings, primsThatApplyTheSchemaKeepTheDataOfUsd)
{
    UsdStageRefPtr stage = makeStage();
    UsdPrim        mesh = UsdGeomMesh::Define(stage, kMesh).GetPrim();
    UsdShadeMaterialBindingAPI::Apply(mesh).Bind(UsdShadeMaterial(stage->GetPrimAtPath(kMatA)));

    Chain chain(stage);

    EXPECT_EQ(chain.boundMaterial(kMesh), kMatA);
    EXPECT_TRUE(chain.materialSurvives(kMatA));
}

TEST(HdVP2LegacyMaterialBindings, onlyPrimsThatBindSomethingGetBindings)
{
    UsdStageRefPtr stage = makeStage();
    UsdGeomMesh::Define(stage, kMesh);
    UsdGeomMesh::Define(stage, SdfPath("/Bound"));
    bindLegacy(stage, SdfPath("/Bound"), kMatA);

    Chain chain(stage);

    EXPECT_FALSE(chain.ownBindings(kMesh)) << "an unbound prim got a bindings container";
    EXPECT_TRUE(chain.ownBindings(SdfPath("/Bound")));

    const HdContainerDataSourceHandle unbound = chain.legacy->GetPrim(kMesh).dataSource;
    const HdContainerDataSourceHandle bound = chain.legacy->GetPrim(SdfPath("/Bound")).dataSource;
    const auto                        has = [](const HdContainerDataSourceHandle& source) {
        const TfTokenVector names = source->GetNames();
        return std::find(
                   names.begin(), names.end(), UsdImagingMaterialBindingsSchema::GetSchemaToken())
            != names.end();
    };
    EXPECT_FALSE(has(unbound));
    EXPECT_TRUE(has(bound));
}

TEST(HdVP2LegacyMaterialBindings, nativeInstanceRootsBindTheirOwnMaterial)
{
    // As LightAPI_Test.usda: the binding is on each instance root, not in the prototype.
    UsdStageRefPtr stage = makeStage();
    UsdGeomXform::Define(stage, SdfPath("/Proto"));
    const SdfPath source("/Proto/Shape");
    UsdGeomMesh::Define(stage, source);
    const std::map<const char*, SdfPath> instances { { "/InstanceA", kMatA },
                                                     { "/InstanceB", kMatB } };
    for (const auto& [path, material] : instances) {
        UsdPrim instance = UsdGeomXform::Define(stage, SdfPath(path)).GetPrim();
        instance.GetReferences().AddInternalReference(SdfPath("/Proto"));
        instance.SetInstanceable(true);
        bindLegacy(stage, SdfPath(path), material);
    }

    Chain chain(stage);

    // The instances differ in their bindings, so each gets a prototype of its own.
    std::set<SdfPath> materials;
    for (const SdfPath& mesh : chain.meshes()) {
        if (mesh != source) {
            materials.insert(chain.boundMaterial(mesh));
        }
    }
    EXPECT_EQ(materials, (std::set<SdfPath> { kMatA, kMatB }));
}

TEST(HdVP2LegacyMaterialBindings, bindingInsideANativeInstancePrototypeIsResolved)
{
    // As texturedBall.usda: the binding is on a prim of the file the instances reference.
    UsdStageRefPtr stage = makeStage();
    UsdGeomXform::Define(stage, SdfPath("/Proto"));
    const SdfPath source("/Proto/Shape");
    UsdGeomMesh::Define(stage, source);
    bindLegacy(stage, source, kMatA);
    for (const char* path : { "/InstanceA", "/InstanceB" }) {
        UsdPrim instance = UsdGeomXform::Define(stage, SdfPath(path)).GetPrim();
        instance.GetReferences().AddInternalReference(SdfPath("/Proto"));
        instance.SetInstanceable(true);
    }

    Chain chain(stage);

    const std::vector<SdfPath> meshes = chain.meshes();
    ASSERT_TRUE(std::any_of(meshes.begin(), meshes.end(), [&](const SdfPath& mesh) {
        return mesh != source;
    })) << "no prototype mesh";
    for (const SdfPath& mesh : meshes) {
        EXPECT_EQ(chain.boundMaterial(mesh), kMatA) << mesh;
    }
}

TEST(HdVP2LegacyMaterialBindings, pointInstancerPrototypeBindingIsResolved)
{
    // As the GLS_Thick prototype of LightAPI_Test.usda.
    UsdStageRefPtr stage = makeStage();
    UsdGeomXform::Define(stage, SdfPath("/Prototypes"));
    const SdfPath source("/Prototypes/Shape");
    UsdGeomMesh::Define(stage, source);
    bindLegacy(stage, source, kMatA);

    UsdGeomPointInstancer instancer = UsdGeomPointInstancer::Define(stage, SdfPath("/PI"));
    instancer.CreatePrototypesRel().AddTarget(source);
    instancer.CreateProtoIndicesAttr().Set(VtIntArray { 0, 0 });
    instancer.CreatePositionsAttr().Set(VtVec3fArray { GfVec3f(0.0f), GfVec3f(5.0f, 0.0f, 0.0f) });

    Chain chain(stage);

    const std::vector<SdfPath> meshes = chain.meshes();
    ASSERT_TRUE(std::any_of(meshes.begin(), meshes.end(), [&](const SdfPath& mesh) {
        return mesh != source;
    })) << "no prototype mesh";
    for (const SdfPath& mesh : meshes) {
        EXPECT_EQ(chain.boundMaterial(mesh), kMatA) << mesh;
    }
}

TEST(HdVP2LegacyMaterialBindings, retargetedBindingIsDirtiedAndResolved)
{
    UsdStageRefPtr stage = makeStage();
    UsdGeomMesh::Define(stage, kMesh);
    UsdRelationship rel = bindLegacy(stage, kMesh, kMatA);
    Chain           chain(stage);
    ASSERT_EQ(chain.boundMaterial(kMesh), kMatA);

    rel.SetTargets({ kMatB });
    chain.applyEdits();

    // Whether kMatA is pruned now is not checked: HdsiUnboundMaterialPruningSceneIndex only ever
    // adds to the materials it counts as bound, with or without the schema.
    EXPECT_EQ(chain.recorder.dirtied[kMesh], 1);
    EXPECT_EQ(chain.boundMaterial(kMesh), kMatB);
    EXPECT_TRUE(chain.materialSurvives(kMatB));
}

TEST(HdVP2LegacyMaterialBindings, newlyAuthoredBindingIsDirtiedAndResolved)
{
    // No relationship spec yet, so the edit arrives as a resync of the property, not as a change.
    UsdStageRefPtr stage = makeStage();
    UsdGeomMesh::Define(stage, kMesh);
    Chain chain(stage);
    ASSERT_TRUE(chain.boundMaterial(kMesh).IsEmpty());

    bindLegacy(stage, kMesh, kMatA);
    chain.applyEdits();

    EXPECT_EQ(chain.recorder.dirtied[kMesh], 1);
    EXPECT_EQ(chain.boundMaterial(kMesh), kMatA);
    EXPECT_TRUE(chain.materialSurvives(kMatA));
}

TEST(HdVP2LegacyMaterialBindings, removedBindingIsDirtiedAndGone)
{
    UsdStageRefPtr stage = makeStage();
    UsdGeomMesh::Define(stage, kMesh);
    bindLegacy(stage, kMesh, kMatA);
    Chain chain(stage);
    ASSERT_EQ(chain.boundMaterial(kMesh), kMatA);

    stage->GetPrimAtPath(kMesh).RemoveProperty(UsdShadeTokens->materialBinding);
    chain.applyEdits();

    EXPECT_EQ(chain.recorder.dirtied[kMesh], 1);
    EXPECT_TRUE(chain.boundMaterial(kMesh).IsEmpty());
}

TEST(HdVP2LegacyMaterialBindings, otherEditsAreNotDirtied)
{
    // Authored up front, so that changing it is a change of value and not a new spec.
    UsdStageRefPtr stage = makeStage();
    UsdGeomMesh    mesh = UsdGeomMesh::Define(stage, kMesh);
    mesh.CreateDoubleSidedAttr(VtValue(true));
    bindLegacy(stage, kMesh, kMatA);
    Chain chain(stage);

    mesh.GetDoubleSidedAttr().Set(false);
    chain.applyEdits();

    EXPECT_TRUE(chain.recorder.dirtied.empty());
}

TEST(HdVP2LegacyMaterialBindings, editOfAPrimThatAppliesTheSchemaIsDirtiedOnce)
{
    // UsdImagingStageSceneIndex dirties this prim itself; doing it again would make every
    // downstream scene index invalidate the bindings of the whole subtree twice.
    UsdStageRefPtr             stage = makeStage();
    UsdPrim                    mesh = UsdGeomMesh::Define(stage, kMesh).GetPrim();
    UsdShadeMaterialBindingAPI api = UsdShadeMaterialBindingAPI::Apply(mesh);
    api.Bind(UsdShadeMaterial(stage->GetPrimAtPath(kMatA)));
    Chain chain(stage);
    ASSERT_EQ(chain.boundMaterial(kMesh), kMatA);

    api.Bind(UsdShadeMaterial(stage->GetPrimAtPath(kMatB)));
    chain.applyEdits();

    EXPECT_EQ(chain.recorder.dirtied[kMesh], 1);
    EXPECT_EQ(chain.boundMaterial(kMesh), kMatB);
}
