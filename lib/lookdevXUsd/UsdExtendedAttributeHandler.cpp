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

#include "UsdExtendedAttributeHandler.h"

#include <mayaUsdAPI/utils.h>

#include <pxr/base/tf/token.h>

namespace LookdevXUsd
{

UsdExtendedAttributeHandler::Ptr UsdExtendedAttributeHandler::create()
{
    return std::make_shared<UsdExtendedAttributeHandler>();
}

bool UsdExtendedAttributeHandler::isAuthoredAttribute(const Ufe::Attribute::Ptr& attribute) const
{
    if (attribute && attribute->sceneItem() && MayaUsdAPI::isUsdSceneItem(attribute->sceneItem()))
    {
        auto prim = MayaUsdAPI::getPrimForUsdSceneItem(attribute->sceneItem());
        return prim.GetAttribute(PXR_NS::TfToken(attribute->name())).IsAuthored();
    }
    return false;
}

} // namespace LookdevXUsd
