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

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/token.h>

namespace LookdevXUsd
{

UsdExtendedAttributeHandler::Ptr UsdExtendedAttributeHandler::create()
{
    return std::make_shared<UsdExtendedAttributeHandler>();
}

bool UsdExtendedAttributeHandler::isAuthoredAttribute(const Ufe::Attribute::Ptr& attribute) const
{
    auto usdItem = attribute ? UsdUfe::downcast(attribute->sceneItem()) : nullptr;
    if (usdItem)
    {
        auto prim = usdItem->prim();
        return prim.GetAttribute(PXR_NS::TfToken(attribute->name())).IsAuthored();
    }
    return false;
}

} // namespace LookdevXUsd
