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
#include <mayaUsd/render/vp2RenderDelegate/sceneIndexProducer.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/unitTestNullRenderDelegate.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>

#include <gtest/gtest.h>

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

const SdfPath kMaterial("/Looks/Red");

enum class Binding
{
    None,
    OnGprim,
    OnParent,
    InNativeInstance
};

// Authors a red material in /Looks, and a cube bound to it as requested.
UsdStageRefPtr makeStage(Binding binding)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    UsdGeomScope::Define(stage, SdfPath("/Looks"));
    UsdShadeMaterial material = UsdShadeMaterial::Define(stage, kMaterial);
    UsdShadeShader   shader = UsdShadeShader::Define(stage, SdfPath("/Looks/Red/Surface"));
    shader.CreateIdAttr().Set(TfToken("UsdPreviewSurface"));
    shader.CreateInput(TfToken("diffuseColor"), SdfValueTypeNames->Color3f)
        .Set(GfVec3f(1.0f, 0.0f, 0.0f));
    material.CreateSurfaceOutput().ConnectToSource(shader.ConnectableAPI(), TfToken("surface"));

    UsdGeomXform::Define(stage, SdfPath("/Geo"));
    UsdGeomCube::Define(stage, SdfPath("/Geo/Cube"));

    switch (binding) {
    case Binding::OnGprim:
        UsdShadeMaterialBindingAPI::Apply(stage->GetPrimAtPath(SdfPath("/Geo/Cube")))
            .Bind(material);
        break;
    case Binding::OnParent:
        UsdShadeMaterialBindingAPI::Apply(stage->GetPrimAtPath(SdfPath("/Geo"))).Bind(material);
        break;
    case Binding::InNativeInstance: {
        // Only the prototype's cube binds the material, and it draws at a prototype path.
        stage->CreateClassPrim(SdfPath("/Prototype"));
        UsdGeomCube::Define(stage, SdfPath("/Prototype/Cube"));
        UsdShadeMaterialBindingAPI::Apply(stage->GetPrimAtPath(SdfPath("/Prototype/Cube")))
            .Bind(material);
        UsdPrim instance = stage->DefinePrim(SdfPath("/Instance"), TfToken("Xform"));
        instance.GetReferences().AddInternalReference(SdfPath("/Prototype"));
        instance.SetInstanceable(true);
        break;
    }
    case Binding::None: break;
    }

    return stage;
}

// A populated producer on a render index that has no renderer.
struct Fixture
{
    // The producer is destroyed first, then the render index it is in, then the delegate.
    Hd_UnitTestNullRenderDelegate            delegate;
    std::unique_ptr<HdRenderIndex>           renderIndex;
    std::unique_ptr<HdVP2SceneIndexProducer> producer;

    explicit Fixture(Binding binding)
        : renderIndex(HdRenderIndex::New(&delegate, HdDriverVector()))
        , producer(new HdVP2SceneIndexProducer())
    {
        producer->Initialize(renderIndex.get(), SdfPath("/Producer"));
        producer->Populate(makeStage(binding), SdfPath::AbsoluteRootPath(), {});
    }

    bool hasMaterial() const
    {
        return renderIndex->GetSprim(HdPrimTypeTokens->material, producer->ToIndexPath(kMaterial))
            != nullptr;
    }
};

// Excluding /Looks removes the material, and clearing the exclusion must bring it back.
void expectMaterialSurvivesExclusion(Binding binding)
{
    Fixture fixture(binding);
    ASSERT_TRUE(fixture.hasMaterial());

    fixture.producer->SetExcludedPaths({ SdfPath("/Looks") });
    EXPECT_FALSE(fixture.hasMaterial());

    fixture.producer->SetExcludedPaths({});
    EXPECT_TRUE(fixture.hasMaterial());
}

} // namespace

// The premise of the tests below: a material nothing binds is pruned.
TEST(HdVP2SceneIndexProducer, unboundMaterialIsPruned)
{
    Fixture fixture(Binding::None);
    EXPECT_FALSE(fixture.hasMaterial());
}

TEST(HdVP2SceneIndexProducer, restoredMaterialBoundOnGprimIsNotPruned)
{
    expectMaterialSurvivesExclusion(Binding::OnGprim);
}

TEST(HdVP2SceneIndexProducer, restoredMaterialBoundOnParentIsNotPruned)
{
    expectMaterialSurvivesExclusion(Binding::OnParent);
}

TEST(HdVP2SceneIndexProducer, restoredMaterialBoundInNativeInstanceIsNotPruned)
{
    expectMaterialSurvivesExclusion(Binding::InNativeInstance);
}
