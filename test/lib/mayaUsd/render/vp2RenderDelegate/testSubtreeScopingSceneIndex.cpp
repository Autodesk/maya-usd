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
#include <pxr/imaging/hd/materialBindingsSchema.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdsi/materialBindingResolvingSceneIndex.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/collectionAPI.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>

#include <gtest/gtest.h>

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
    HdVP2SubtreeScopingSceneIndexRefPtr& scoping)
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
