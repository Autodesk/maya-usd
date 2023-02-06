//
// Copyright 2022 Autodesk
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
#include "UsdMaterial.h"

#include <mayaUsd/ufe/Utils.h>

#include <ufe/types.h>

#include <stdexcept>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

PXR_NS::UsdPrim getAncestorMaterial(const PXR_NS::UsdPrim& prim)
{
    // Make sure we're returning a Material, which is the (grand)parent of a Shader or NodeGraph.

    if (prim.GetTypeName() == "Material")
    {
        return prim;
    }

    auto iterPrim = prim;
    while (iterPrim.GetTypeName() == "Shader" || iterPrim.GetTypeName() == "NodeGraph")
    {
        iterPrim = iterPrim.GetParent();
    }
    if (iterPrim)
    {
        return iterPrim;
    }

    return PXR_NS::UsdPrim();
}

std::vector<PXR_NS::UsdPrim> getConnectedShaders(const PXR_NS::UsdShadeOutput& port)
{
    // Dig down across NodeGraph boundaries until a surface shader is found.
    std::vector<PXR_NS::UsdPrim> shaders;
    for (const auto& sourceInfo : port.GetConnectedSources()) {
        if (sourceInfo.source.GetPrim().GetTypeName() != "Shader") {
            if (sourceInfo.sourceType == UsdShadeAttributeType::Output) {
                const std::vector<PXR_NS::UsdPrim> connectedShaders
                    = getConnectedShaders(sourceInfo.source.GetOutput(sourceInfo.sourceName));
                shaders.insert(
                    std::end(shaders), std::begin(connectedShaders), std::end(connectedShaders));
            }
        } else {
            shaders.push_back(sourceInfo.source.GetPrim());
        }
    }
    return shaders;
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdMaterial::UsdMaterial(const UsdSceneItem::Ptr& item)
    : Ufe::Material()
    , _item(item)
{
}

UsdMaterial::~UsdMaterial() { }

/*static*/
UsdMaterial::Ptr UsdMaterial::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdMaterial>(item);
}

std::vector<Ufe::SceneItem::Ptr> UsdMaterial::getMaterials() const
{
    // Find the material(s) attached to our SceneItem.

    std::vector<Ufe::SceneItem::Ptr> materials;

    if (!TF_VERIFY(_item)) {
        return materials;
    }

    std::vector<PXR_NS::UsdPrim> shaderPrims;

    const PXR_NS::UsdPrim&                            prim = _item->prim();
    PXR_NS::UsdShadeMaterialBindingAPI                bindingApi(prim);
    PXR_NS::UsdShadeMaterialBindingAPI::DirectBinding directBinding = bindingApi.GetDirectBinding();

    // 1. Simple case: A material is directly attached to our object.
    const PXR_NS::UsdShadeMaterial material = directBinding.GetMaterial();
    if (material) {
        for (const auto& output : material.GetSurfaceOutputs()) {
            for (const auto& shader : getConnectedShaders(output)) {
                shaderPrims.push_back(shader);
            }
        }
    }

    // 2. Check whether multiple materials are attached to this object via geometry subsets.
    for (const auto& geometrySubset : bindingApi.GetMaterialBindSubsets()) {
        const UsdShadeMaterialBindingAPI subsetBindingAPI(geometrySubset.GetPrim());
        const UsdShadeMaterial           material
            = subsetBindingAPI.ComputeBoundMaterial(UsdShadeTokens->surface);
        if (material) {
            for (const auto& output : material.GetSurfaceOutputs()) {
                for (const auto& shader : getConnectedShaders(output)) {
                    shaderPrims.push_back(shader);
                }
            }
        }
    }

    // 3. Find the associated Ufe::SceneItem for each material attached to our object.
    for (auto& shaderPrim : shaderPrims) {
        if (!shaderPrim) {
            continue;
        }

        // Make sure we're working with the material, not an internal shader.
        const auto& materialPrim = getAncestorMaterial(shaderPrim);
        if (!materialPrim) {
            continue;
        }

        const PXR_NS::SdfPath& materialSdfPath = materialPrim.GetPath();
        const Ufe::Path        materialUfePath = usdPathToUfePathSegment(materialSdfPath);

        // Construct a UFE path consisting of two segments:
        // 1. The path to the USD stage
        // 2. The path to our material
        const auto stagePathSegments = _item->path().getSegments();
        const auto materialPathSegments = materialUfePath.getSegments();
        if (stagePathSegments.empty() || materialPathSegments.empty())
            continue;
        const auto ufePath = Ufe::Path({ stagePathSegments[0], materialPathSegments[0] });

        // Now we have the full path to the material's SceneItem.
        materials.push_back(UsdSceneItem::create(ufePath, materialPrim));
    }

    return materials;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
