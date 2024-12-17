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

#include "UsdSceneItemUI.h"
#include <LookdevXUfe/SoloingHandler.h>
#include <LookdevXUfe/UfeUtils.h>
#include <LookdevXUfe/Utils.h>

#include <mayaUsdAPI/utils.h>

namespace
{

// For backwards compatibility for when we used usd prim hidden flag. Needs to be removed at some point.
bool isLegacyHiddenItem(const Ufe::SceneItem::Ptr& item)
{
    if (!item)
    {
        return false;
    }
    const auto soloingHandler = LookdevXUfe::SoloingHandler::get(item->runTimeId());
    const auto nodeDef = LookdevXUfe::UfeUtils::getNodeDef(item);
    const auto isSeparateOrCombine =
        nodeDef ? LookdevXUfe::identifyComponentNode(nodeDef->type()) != LookdevXUfe::ComponentNodeType::eNone : false;

    auto prim = MayaUsdAPI::getPrimForUsdSceneItem(item);
    return (prim.IsValid() && prim.IsHidden()) &&
           (isSeparateOrCombine || (soloingHandler && soloingHandler->isSoloingItem(item)));
}

} // namespace

namespace LookdevXUsd
{

UsdSceneItemUI::UsdSceneItemUI(Ufe::SceneItem::Ptr item, const PXR_NS::UsdPrim& prim)
    : m_item(std::move(item)), m_stage(prim.IsValid() ? prim.GetStage() : nullptr),
      m_path(prim.IsValid() ? prim.GetPath() : PXR_NS::SdfPath())
{
}

/*static*/
UsdSceneItemUI::Ptr UsdSceneItemUI::create(const Ufe::SceneItem::Ptr& item)
{
    auto prim = MayaUsdAPI::getPrimForUsdSceneItem(item);
    return std::make_shared<UsdSceneItemUI>(item, prim);
}

//------------------------------------------------------------------------------
// Ufe::SceneItemUI overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdSceneItemUI::sceneItem() const
{
    return m_item;
}

bool UsdSceneItemUI::hidden() const
{
    return isLegacyHiddenItem(m_item) ||
           LookdevXUfe::getAutodeskMetadata(m_item, getHiddenKeyMetadata(m_item)).get<std::string>() == "true";
}

Ufe::UndoableCommand::Ptr UsdSceneItemUI::setHiddenCmd(bool hidden)
{
    return LookdevXUfe::setAutodeskMetadataCmd(m_item, getHiddenKeyMetadata(m_item),
                                               hidden ? std::string("true") : std::string("false"));
}

} // namespace LookdevXUsd
