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
#include <mayaUsd/render/vp2RenderDelegate/sceneIndex/polygonalNormalsSceneIndex.h>

#include <pxr/imaging/hd/changeTracker.h>
#include <pxr/imaging/hd/dirtyBitsTranslator.h>
#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/primvarSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/retainedSceneIndex.h>
#include <pxr/imaging/hd/sceneIndexPrimView.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// What the render index would be told is dirty, per prim.
class DirtyRecorder : public HdSceneIndexObserver
{
public:
    std::map<SdfPath, HdDataSourceLocatorSet> dirtied;

    void PrimsAdded(const HdSceneIndexBase&, const AddedPrimEntries&) override { }
    void PrimsRemoved(const HdSceneIndexBase&, const RemovedPrimEntries&) override { }
    void PrimsDirtied(const HdSceneIndexBase&, const DirtiedPrimEntries& entries) override
    {
        for (const DirtiedPrimEntry& entry : entries) {
            dirtied[entry.primPath].insert(entry.dirtyLocators);
        }
    }
    void PrimsRenamed(const HdSceneIndexBase&, const RenamedPrimEntries&) override { }
};

// The primvar names of a prim, as the emulation delegate lists them for the render index.
TfTokenVector primvarNames(const HdSceneIndexBaseRefPtr& scene, const SdfPath& path)
{
    return HdPrimvarsSchema::GetFromParent(scene->GetPrim(path).dataSource).GetPrimvarNames();
}

// Whether normals are among the primvar names, and whether they can be looked up. UsdImaging
// lists them for every mesh, with or without normals authored, as it lists every primvar it can
// map, so a mesh that has none can be listed and not found. The emulation delegate skips those.
struct NormalsView
{
    bool listed;
    bool found;
};

NormalsView normalsView(const HdSceneIndexBaseRefPtr& scene, const SdfPath& path)
{
    const TfTokenVector names = primvarNames(scene, path);
    return { std::find(names.begin(), names.end(), HdPrimvarsSchemaTokens->normals) != names.end(),
             HdPrimvarsSchema::GetFromParent(scene->GetPrim(path).dataSource)
                 .GetPrimvar(HdPrimvarsSchemaTokens->normals)
                 .IsDefined() };
}

bool keepsNormals(const HdSceneIndexBaseRefPtr& scene, const SdfPath& path)
{
    const NormalsView view = normalsView(scene, path);
    return view.listed && view.found;
}

// Neither listed nor found.
bool hidesNormals(const HdSceneIndexBaseRefPtr& scene, const SdfPath& path)
{
    const NormalsView view = normalsView(scene, path);
    return !view.listed && !view.found;
}

// The producer's chain, from the stage scene index to what its own filters are given: the
// polygonal normals scene index on top of the one UsdImaging builds.
struct Chain
{
    explicit Chain(const UsdStageRefPtr& stage_)
        : stage(stage_)
    {
        const UsdImagingSceneIndices indices
            = UsdImagingCreateSceneIndices(UsdImagingCreateSceneIndicesInfo());
        stageSceneIndex = indices.stageSceneIndex;
        end = HdVP2PolygonalNormalsSceneIndex::New(indices.finalSceneIndex);
        end->AddObserver(HdSceneIndexObserverPtr(&recorder));
        stageSceneIndex->SetStage(stage);
    }

    // As HdVP2SceneIndexProducer::ApplyPendingUpdates does.
    void applyEdits() { stageSceneIndex->ApplyPendingUpdates(); }

    bool keepsNormals(const SdfPath& path) const { return ::keepsNormals(end, path); }
    bool hidesNormals(const SdfPath& path) const { return ::hidesNormals(end, path); }

    // Whether the dirty notices reported the normals of the prim.
    bool normalsDirtied(const SdfPath& path) const
    {
        const auto it = recorder.dirtied.find(path);
        return it != recorder.dirtied.end()
            && it->second.Intersects(HdPrimvarsSchema::GetNormalsLocator());
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

    UsdStageRefPtr                  stage;
    UsdImagingStageSceneIndexRefPtr stageSceneIndex;
    HdSceneIndexBaseRefPtr          end;
    DirtyRecorder                   recorder;
};

const SdfPath kMesh("/Mesh");

VtVec3fArray someNormals()
{
    return VtVec3fArray { GfVec3f(0, 0, 1), GfVec3f(0, 0, 1), GfVec3f(0, 0, 1) };
}

// A triangle, with the scheme authored if there is one.
UsdGeomMesh defineMesh(const UsdStageRefPtr& stage, const SdfPath& path, const char* scheme)
{
    UsdGeomMesh mesh = UsdGeomMesh::Define(stage, path);
    mesh.CreatePointsAttr(VtValue(VtVec3fArray { GfVec3f(0), GfVec3f(1, 0, 0), GfVec3f(0, 1, 0) }));
    mesh.CreateFaceVertexCountsAttr(VtValue(VtIntArray { 3 }));
    mesh.CreateFaceVertexIndicesAttr(VtValue(VtIntArray { 0, 1, 2 }));
    if (scheme) {
        mesh.CreateSubdivisionSchemeAttr(VtValue(TfToken(scheme)));
    }
    return mesh;
}

// primvars:normals, the preferred form.
void authorNormalsPrimvar(
    const UsdPrim&      prim,
    const TfToken&      interpolation = UsdGeomTokens->faceVarying,
    const VtVec3fArray& normals = someNormals())
{
    UsdGeomPrimvar primvar = UsdGeomPrimvarsAPI(prim).CreatePrimvar(
        TfToken("normals"), SdfValueTypeNames->Normal3fArray, interpolation);
    primvar.Set(normals);
}

// The normals attribute, the weaker form.
void authorNormalsAttribute(UsdGeomMesh mesh)
{
    mesh.CreateNormalsAttr(VtValue(someNormals()));
    mesh.SetNormalsInterpolation(UsdGeomTokens->faceVarying);
}

// A mesh with normals at /Mesh.
UsdStageRefPtr makeStage(const char* scheme, bool asAttribute = false)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomMesh    mesh = defineMesh(stage, kMesh, scheme);
    if (asAttribute) {
        authorNormalsAttribute(mesh);
    } else {
        authorNormalsPrimvar(mesh.GetPrim());
    }
    return stage;
}

// A retained scene with one prim of the given type, publishing normals and the scheme, if any.
HdSceneIndexBaseRefPtr makeRetainedScene(const TfToken& primType, const TfToken& scheme)
{
    const HdContainerDataSourceHandle normals
        = HdPrimvarSchema::Builder()
              .SetPrimvarValue(HdRetainedTypedSampledDataSource<VtVec3fArray>::New(someNormals()))
              .SetInterpolation(
                  HdPrimvarSchema::BuildInterpolationDataSource(HdPrimvarSchemaTokens->faceVarying))
              .SetRole(HdPrimvarSchema::BuildRoleDataSource(HdPrimvarSchemaTokens->normal))
              .Build();

    std::vector<HdDataSourceBaseHandle> children;
    std::vector<TfToken>                names;
    names.push_back(HdPrimvarsSchema::GetSchemaToken());
    children.push_back(
        HdRetainedContainerDataSource::New(HdPrimvarsSchemaTokens->normals, normals));
    if (!scheme.IsEmpty()) {
        names.push_back(HdMeshSchema::GetSchemaToken());
        children.push_back(HdRetainedContainerDataSource::New(
            HdMeshSchemaTokens->subdivisionScheme,
            HdRetainedTypedSampledDataSource<TfToken>::New(scheme)));
    }

    HdRetainedSceneIndexRefPtr scene = HdRetainedSceneIndex::New();
    scene->AddPrims(
        { { kMesh,
            primType,
            HdRetainedContainerDataSource::New(names.size(), names.data(), children.data()) } });
    return HdVP2PolygonalNormalsSceneIndex::New(scene);
}

} // namespace

TEST(HdVP2PolygonalNormals, normalsOfASubdividedMeshAreHidden)
{
    // No scheme authored is catmullClark, the schema's fallback.
    for (const char* scheme :
         { static_cast<const char*>(nullptr), "catmullClark", "loop", "bilinear" }) {
        Chain chain(makeStage(scheme));
        EXPECT_TRUE(chain.hidesNormals(kMesh)) << (scheme ? scheme : "no scheme authored");
    }
}

TEST(HdVP2PolygonalNormals, normalsOfAPolygonalMeshAreKept)
{
    Chain chain(makeStage("none"));
    EXPECT_TRUE(chain.keepsNormals(kMesh));
}

TEST(HdVP2PolygonalNormals, theNormalsAttributeFollowsTheSameRule)
{
    {
        Chain chain(makeStage(nullptr, /* asAttribute = */ true));
        EXPECT_TRUE(chain.hidesNormals(kMesh));
    }
    {
        Chain chain(makeStage("none", /* asAttribute = */ true));
        EXPECT_TRUE(chain.keepsNormals(kMesh));
    }
}

TEST(HdVP2PolygonalNormals, inheritedNormalsFollowTheSameRule)
{
    // Constant primvars are inherited down the namespace.
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim        group = UsdGeomXform::Define(stage, SdfPath("/Group")).GetPrim();
    authorNormalsPrimvar(group, UsdGeomTokens->constant, VtVec3fArray { GfVec3f(0, 0, 1) });
    const SdfPath subdivided("/Group/Subdivided");
    const SdfPath polygonal("/Group/Polygonal");
    defineMesh(stage, subdivided, nullptr);
    defineMesh(stage, polygonal, "none");

    Chain chain(stage);

    EXPECT_TRUE(chain.keepsNormals(polygonal)) << "the normals were not inherited";
    EXPECT_TRUE(chain.hidesNormals(subdivided));
}

TEST(HdVP2PolygonalNormals, otherPrimvarsAreKept)
{
    UsdStageRefPtr stage = makeStage(nullptr);
    UsdPrim        mesh = stage->GetPrimAtPath(kMesh);
    UsdGeomPrimvarsAPI(mesh).CreatePrimvar(
        TfToken("st"), SdfValueTypeNames->TexCoord2fArray, UsdGeomTokens->faceVarying);
    UsdGeomPrimvarsAPI(mesh).CreatePrimvar(
        TfToken("displayColor"), SdfValueTypeNames->Color3fArray, UsdGeomTokens->constant);
    Chain chain(stage);

    const TfTokenVector names = primvarNames(chain.end, kMesh);
    for (const char* name : { "st", "displayColor", "points" }) {
        EXPECT_TRUE(std::find(names.begin(), names.end(), TfToken(name)) != names.end()) << name;
    }
    EXPECT_TRUE(chain.hidesNormals(kMesh));
}

TEST(HdVP2PolygonalNormals, nativeInstancePrototypeFollowsTheRule)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform::Define(stage, SdfPath("/Proto"));
    const SdfPath source("/Proto/Mesh");
    authorNormalsPrimvar(defineMesh(stage, source, nullptr).GetPrim());
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
        EXPECT_TRUE(chain.hidesNormals(mesh)) << mesh;
    }
}

TEST(HdVP2PolygonalNormals, pointInstancerPrototypeFollowsTheRule)
{
    // As point_instanced_axis.usda: the glyph authors normals and no scheme.
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform::Define(stage, SdfPath("/Prototypes"));
    const SdfPath source("/Prototypes/Glyph");
    authorNormalsPrimvar(defineMesh(stage, source, nullptr).GetPrim());

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
        EXPECT_TRUE(chain.hidesNormals(mesh)) << mesh;
    }
}

TEST(HdVP2PolygonalNormals, editingTheSchemeDirtiesTheNormals)
{
    UsdStageRefPtr stage = makeStage("none");
    Chain          chain(stage);
    ASSERT_TRUE(chain.keepsNormals(kMesh));
    UsdGeomMesh mesh(stage->GetPrimAtPath(kMesh));

    mesh.GetSubdivisionSchemeAttr().Set(UsdGeomTokens->catmullClark);
    chain.applyEdits();

    EXPECT_TRUE(chain.hidesNormals(kMesh));
    ASSERT_TRUE(chain.normalsDirtied(kMesh)) << "the render index was not told";
    // The edit alone is a change of topology to the emulation delegate. It takes the normals to
    // be dirty to ask for the primvars again.
    EXPECT_TRUE(
        HdDirtyBitsTranslator::RprimLocatorSetToDirtyBits(
            HdPrimTypeTokens->mesh, chain.recorder.dirtied[kMesh])
        & HdChangeTracker::DirtyNormals);

    // And back.
    chain.recorder.dirtied.clear();
    mesh.GetSubdivisionSchemeAttr().Set(UsdGeomTokens->none);
    chain.applyEdits();

    EXPECT_TRUE(chain.keepsNormals(kMesh));
    EXPECT_TRUE(chain.normalsDirtied(kMesh));
}

TEST(HdVP2PolygonalNormals, authoringTheSchemeDirtiesTheNormals)
{
    // No scheme authored yet, so the edit arrives as a resync of the property.
    UsdStageRefPtr stage = makeStage(nullptr);
    Chain          chain(stage);
    ASSERT_TRUE(chain.hidesNormals(kMesh));

    UsdGeomMesh(stage->GetPrimAtPath(kMesh))
        .CreateSubdivisionSchemeAttr(VtValue(UsdGeomTokens->none));
    chain.applyEdits();

    EXPECT_TRUE(chain.keepsNormals(kMesh));
    EXPECT_TRUE(chain.normalsDirtied(kMesh));
}

TEST(HdVP2PolygonalNormals, otherEditsAreNotDirtiedAsNormals)
{
    UsdStageRefPtr stage = makeStage("none");
    UsdGeomMesh    mesh(stage->GetPrimAtPath(kMesh));
    mesh.CreateDoubleSidedAttr(VtValue(true));
    Chain chain(stage);

    mesh.GetDoubleSidedAttr().Set(false);
    chain.applyEdits();

    EXPECT_FALSE(chain.normalsDirtied(kMesh));
}

TEST(HdVP2PolygonalNormals, editsOfTheNormalsOfAPolygonalMeshFlowThrough)
{
    UsdStageRefPtr stage = makeStage("none");
    Chain          chain(stage);

    UsdGeomPrimvarsAPI(stage->GetPrimAtPath(kMesh))
        .GetPrimvar(TfToken("normals"))
        .Set(VtVec3fArray { GfVec3f(1, 0, 0), GfVec3f(1, 0, 0), GfVec3f(1, 0, 0) });
    chain.applyEdits();

    EXPECT_TRUE(chain.normalsDirtied(kMesh));
    EXPECT_TRUE(chain.keepsNormals(kMesh));
}

TEST(HdVP2PolygonalNormals, meshWithoutASchemeIsPolygonal)
{
    // The emulation delegate takes the scheme to be none where none is published.
    const HdSceneIndexBaseRefPtr scene = makeRetainedScene(HdPrimTypeTokens->mesh, TfToken());
    EXPECT_TRUE(keepsNormals(scene, kMesh));
}

TEST(HdVP2PolygonalNormals, schemeOfARetainedMeshDecides)
{
    EXPECT_TRUE(hidesNormals(
        makeRetainedScene(HdPrimTypeTokens->mesh, UsdGeomTokens->catmullClark), kMesh));
    EXPECT_TRUE(
        keepsNormals(makeRetainedScene(HdPrimTypeTokens->mesh, UsdGeomTokens->none), kMesh));
}

TEST(HdVP2PolygonalNormals, otherPrimTypesAreLeftAlone)
{
    // Only a mesh has a scheme to go by, whatever a prim of another type publishes.
    const HdSceneIndexBaseRefPtr scene
        = makeRetainedScene(HdPrimTypeTokens->basisCurves, UsdGeomTokens->catmullClark);
    EXPECT_TRUE(keepsNormals(scene, kMesh));
}
