//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#include "UsdMaterialHandler.h"

#include "UsdMaterial.h"
#include "UsdMaterialValidator.h"
#include "UsdMxVersionUpgrade.h"

#include <mayaUsdAPI/utils.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/subset.h>

namespace LookdevXUsd
{

UsdMaterialHandler::UsdMaterialHandler() : LookdevXUfe::MaterialHandler()
{
}

UsdMaterialHandler::~UsdMaterialHandler()
{
}

//------------------------------------------------------------------------------
// UsdMaterialHandler overrides
//------------------------------------------------------------------------------

LookdevXUfe::Material::Ptr UsdMaterialHandler::material(const Ufe::SceneItem::Ptr& item) const
{
    using namespace PXR_NS;

    if (!TF_VERIFY(MayaUsdAPI::isUsdSceneItem(item), "Invalid item\n"))
    {
        return nullptr;
    }

    // Test if this item is imageable or a geom subset. If not, then we cannot create a material
    // interface for it, which is a valid case (such as for a material node type).
    const auto prim = MayaUsdAPI::getPrimForUsdSceneItem(item);
    if (!UsdGeomImageable(prim) && !prim.IsA<UsdGeomSubset>())
    {
        return nullptr;
    }

    return UsdMaterial::create(item);
}

Ufe::SceneItemResultUndoableCommand::Ptr UsdMaterialHandler::createBackdropCmdImpl(const Ufe::SceneItem::Ptr& parent,
                                                                                   const Ufe::PathComponent& name) const
{
    if (!MayaUsdAPI::isUsdSceneItem(parent) || (parent->nodeType() != "NodeGraph" && parent->nodeType() != "Material"))
    {
        return nullptr;
    }

    return MayaUsdAPI::createAddNewPrimCommand(parent, name.string(), "Backdrop");
}

Ufe::SceneItemResultUndoableCommand::Ptr UsdMaterialHandler::createNodeGraphCmdImpl(
    const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name) const
{
    return MayaUsdAPI::createAddNewPrimCommand(parent, name.string(), "NodeGraph");
}

LookdevXUfe::ValidationLog::Ptr UsdMaterialHandler::validateMaterial(const Ufe::SceneItem::Ptr& material) const
{
    if (!MayaUsdAPI::isUsdSceneItem(material))
    {
        return nullptr;
    }

    auto materialPrim = PXR_NS::UsdShadeMaterial(MayaUsdAPI::getPrimForUsdSceneItem(material));
    if (!materialPrim)
    {
        return nullptr;
    }

    return UsdMaterialValidator(materialPrim).validate();
}

bool UsdMaterialHandler::isBackdropImpl(const Ufe::SceneItem::Ptr& item) const
{
    return item->nodeType() == "Backdrop";
}

bool UsdMaterialHandler::isNodeGraphImpl(const Ufe::SceneItem::Ptr& item) const
{
    return item->nodeType() == "NodeGraph";
}

bool UsdMaterialHandler::isMaterialImpl(const Ufe::SceneItem::Ptr& item) const
{
    return item->nodeType() == "Material";
}

bool UsdMaterialHandler::isShaderImpl(const Ufe::SceneItem::Ptr& item) const
{
    return item->nodeType() == "Shader";
}

bool UsdMaterialHandler::allowedInNodeGraph(const std::string& /*nodeDefType*/) const
{
    return true;
}

#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
std::optional<std::string> UsdMaterialHandler::isLegacyShaderGraph(const Ufe::SceneItem::Ptr& graphElement) const
{
    return LookdevXUsd::Version::isLegacyShaderGraph(graphElement->path());
}

Ufe::UndoableCommand::Ptr UsdMaterialHandler::upgradeLegacyShaderGraphCmd(const Ufe::SceneItem::Ptr& graphElement) const
{
    return LookdevXUsd::Version::UsdMxUpgradeMaterialCmd::create(graphElement->path());
}
#endif

} // namespace LookdevXUsd
