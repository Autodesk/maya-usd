//*****************************************************************************
// Copyright (c) 2023 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#include "UsdMaterial.h"

#include <mayaUsdAPI/utils.h>

#include <ufe/types.h>

#include <pxr/base/tf/diagnostic.h>

using namespace PXR_NS;

namespace LookdevXUsd
{

UsdMaterial::UsdMaterial(Ufe::SceneItem::Ptr item) : m_item(std::move(item))
{
}

UsdMaterial::~UsdMaterial()
= default;

/*static*/
UsdMaterial::Ptr UsdMaterial::create(const Ufe::SceneItem::Ptr& item)
{
    return std::make_shared<UsdMaterial>(item);
}

std::vector<Ufe::SceneItem::Ptr> UsdMaterial::getMaterials() const
{
    // Find the material(s) attached to our SceneItem.

    std::vector<Ufe::SceneItem::Ptr> materials;

    if (!TF_VERIFY(m_item, "Invalid item\n"))
    {
        return materials;
    }

    std::vector<PXR_NS::UsdPrim> materialPrims;

    const PXR_NS::UsdPrim& prim = MayaUsdAPI::getPrimForUsdSceneItem(m_item);
    PXR_NS::UsdShadeMaterialBindingAPI bindingApi(prim);

    // 1. Simple case: A material is directly attached to our object.
    PXR_NS::UsdShadeMaterialBindingAPI::DirectBinding directBinding = bindingApi.GetDirectBinding();
    const PXR_NS::UsdShadeMaterial material = directBinding.GetMaterial();
    if (material)
    {
        materialPrims.push_back(material.GetPrim());
    }

    // 2. Check whether multiple materials are attached to this object via geometry subsets.
    for (const auto& geometrySubset : bindingApi.GetMaterialBindSubsets())
    {
        const PXR_NS::UsdShadeMaterialBindingAPI subsetBindingAPI(geometrySubset.GetPrim());
        const PXR_NS::UsdShadeMaterial materialOnSubset =
            subsetBindingAPI.ComputeBoundMaterial(UsdShadeTokens->surface);
        if (materialOnSubset)
        {
            materialPrims.push_back(materialOnSubset.GetPrim());
        }
    }

    // 3. Find the associated Ufe::SceneItem for each material attached to our object.
    for (auto& materialPrim : materialPrims)
    {
        const PXR_NS::SdfPath& materialSdfPath = materialPrim.GetPath();
        const Ufe::Path materialUfePath = MayaUsdAPI::usdPathToUfePathSegment(materialSdfPath);

        // Construct a UFE path consisting of two segments:
        // 1. The path to the USD stage
        // 2. The path to our material
        const auto stagePathSegments = m_item->path().getSegments();
        const auto& materialPathSegments = materialUfePath.getSegments();
        if (stagePathSegments.empty() || materialPathSegments.empty())
        {
            continue;
        }

        const auto ufePath = Ufe::Path({stagePathSegments[0], materialPathSegments[0]});

        // Now we have the full path to the material's SceneItem.
        materials.push_back(MayaUsdAPI::createUsdSceneItem(ufePath, materialPrim));
    }

    return materials;
}

bool UsdMaterial::hasMaterial() const
{
    if (!TF_VERIFY(m_item, "Invalid item\n"))
    {
        return false;
    }

    const PXR_NS::UsdPrim& prim = MayaUsdAPI::getPrimForUsdSceneItem(m_item);
    PXR_NS::UsdShadeMaterialBindingAPI bindingApi(prim);

    // 1. Simple case: A material is directly attached to our object.
    PXR_NS::UsdShadeMaterialBindingAPI::DirectBinding directBinding = bindingApi.GetDirectBinding();
    const PXR_NS::UsdShadeMaterial material = directBinding.GetMaterial();
    if (material)
    {
        return true;
    }

    // 2. Check whether any material is attached to this object via geometry subsets.
    for (const auto& geometrySubset : bindingApi.GetMaterialBindSubsets())
    {
        const PXR_NS::UsdShadeMaterialBindingAPI subsetBindingAPI(geometrySubset.GetPrim());
        const PXR_NS::UsdShadeMaterial materialOnSubset =
            subsetBindingAPI.ComputeBoundMaterial(UsdShadeTokens->surface);
        if (materialOnSubset)
        {
            return true;
        }
    }

    return false;
}

} // namespace LookdevXUsd
