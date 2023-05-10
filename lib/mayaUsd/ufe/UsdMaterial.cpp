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

    std::vector<PXR_NS::UsdPrim> materialPrims;

    const PXR_NS::UsdPrim&             prim = _item->prim();
    PXR_NS::UsdShadeMaterialBindingAPI bindingApi(prim);

    // 1. Simple case: A material is directly attached to our object.
    PXR_NS::UsdShadeMaterialBindingAPI::DirectBinding directBinding = bindingApi.GetDirectBinding();
    const PXR_NS::UsdShadeMaterial                    material = directBinding.GetMaterial();
    if (material) {
        materialPrims.push_back(material.GetPrim());
    }

    // 2. Check whether multiple materials are attached to this object via geometry subsets.
    for (const auto& geometrySubset : bindingApi.GetMaterialBindSubsets()) {
        const PXR_NS::UsdShadeMaterialBindingAPI subsetBindingAPI(geometrySubset.GetPrim());
        const PXR_NS::UsdShadeMaterial           material
            = subsetBindingAPI.ComputeBoundMaterial(UsdShadeTokens->surface);
        if (material) {
            materialPrims.push_back(material.GetPrim());
        }
    }

    // 3. Find the associated Ufe::SceneItem for each material attached to our object.
    for (auto& materialPrim : materialPrims) {
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

#if (UFE_PREVIEW_VERSION_NUM >= 5003)

bool UsdMaterial::hasMaterial() const
{
    if (!TF_VERIFY(_item)) {
        return false;
    }

    const PXR_NS::UsdPrim&             prim = _item->prim();
    PXR_NS::UsdShadeMaterialBindingAPI bindingApi(prim);

    // 1. Simple case: A material is directly attached to our object.
    PXR_NS::UsdShadeMaterialBindingAPI::DirectBinding directBinding = bindingApi.GetDirectBinding();
    const PXR_NS::UsdShadeMaterial                    material = directBinding.GetMaterial();
    if (material) {
        return true;
    }

    // 2. Check whether any material is attached to this object via geometry subsets.
    for (const auto& geometrySubset : bindingApi.GetMaterialBindSubsets()) {
        const PXR_NS::UsdShadeMaterialBindingAPI subsetBindingAPI(geometrySubset.GetPrim());
        const PXR_NS::UsdShadeMaterial           material
            = subsetBindingAPI.ComputeBoundMaterial(UsdShadeTokens->surface);
        if (material) {
            return true;
        }
    }

    return false;
}

#endif

#if (UFE_PREVIEW_VERSION_NUM >= 5005)

bool UsdMaterial::canAssignMaterial() const
{
    if (!TF_VERIFY(_item)) {
        return false;
    }

    // todo

    return false;
}

#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
