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

#include "UsdComponentConnections.h"

#include <LookdevXUfe/UfeUtils.h>
#include <ufe/runTimeMgr.h>

namespace LookdevXUsd
{

UsdComponentConnections::UsdComponentConnections(const Ufe::SceneItem::Ptr& item) : ComponentConnections(item)
{
}

UsdComponentConnections::Ptr UsdComponentConnections::create(const Ufe::SceneItem::Ptr& item)
{
    return std::make_shared<UsdComponentConnections>(item);
}

std::vector<std::string> UsdComponentConnections::componentNames(const Ufe::Attribute::Ptr& attr) const
{
    return LookdevXUfe::UfeUtils::attributeComponentsAsStrings(attr);
}

Ufe::Connections::Ptr UsdComponentConnections::connections(const Ufe::SceneItem::Ptr& sceneItem) const
{
    auto connHandler = Ufe::RunTimeMgr::instance().connectionHandler(sceneItem->runTimeId());
    if (!connHandler)
    {
        return nullptr;
    }
    return connHandler->sourceConnections(sceneItem);
}

} // namespace LookdevXUsd
